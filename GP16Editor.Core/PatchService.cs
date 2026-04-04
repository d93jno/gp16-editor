using System.Diagnostics;
using System.Text;
using System.Text.RegularExpressions;
using GP16Editor.Models;
using Melanchall.DryWetMidi.Core;

namespace GP16Editor.Core
{
    public class PatchService(MidiService midiService, SysExService sysExService)

    {
        private readonly MidiService _midiService = midiService;
        private readonly SysExService _sysExService = sysExService;

        private const int PATCH_SIZE = 0x7F;
        private const int PATCH_OFFSET = 0x80;
        private const int PATCHES_PER_GROUP = 64;
        private const int TOTAL_PATCHES = 128;


        public async Task<List<Patch>> GetAllPatchesAsync()
        {
            return await GetAllPatchesAsync(null);
        }

        public static string HexDump(byte[] bytes)
        {
            if (bytes == null || bytes.Length == 0)
            {
                return "";
            }

            var sb = new StringBuilder();
            for (int i = 0; i < bytes.Length; i += 16)
            {
                sb.AppendFormat("{0:x8}: ", i);
                for (int j = 0; j < 16; j++)
                {
                    if (i + j < bytes.Length)
                    {
                        sb.AppendFormat("{0:x2} ", bytes[i + j]);
                    }
                    else
                    {
                        sb.Append("   ");
                    }
                }
                sb.Append(" ");
                for (int j = 0; j < 16; j++)
                {
                    if (i + j < bytes.Length)
                    {
                        char c = (char)bytes[i + j];
                        sb.Append(char.IsControl(c) ? '.' : c);
                    }
                }
                sb.AppendLine();
            }
            return sb.ToString();
        }

        public async Task<List<Patch>> GetAllPatchesAsync(IProgress<int>? progress, IProgress<int>? byteProgress = null)
        {
            return await GetPatchesAsync(includeGroupA: true, includeGroupB: true, progress, byteProgress);
        }

        public async Task<List<Patch>> GetPatchesAsync(bool includeGroupA, bool includeGroupB, IProgress<int>? progress = null, IProgress<int>? byteProgress = null)
        {
            if (!includeGroupA && !includeGroupB)
            {
                throw new ArgumentException("At least one patch group must be selected.");
            }

            var patches = new List<Patch>();
            var tcs = new TaskCompletionSource<bool>();
            var byteBuffer = new List<byte>();
            var totalByteOffset = 0;
            var totalExpectedBytes = ((includeGroupA ? 1 : 0) + (includeGroupB ? 1 : 0)) * 8192;

            void SysExHandler(object? sender, NormalSysExEvent e)
            {
                // Reconstruct the full message correctly
                var fullMsg = new List<byte>();
                if (e.Data[0] != 0xF0) fullMsg.Add(0xF0);
                fullMsg.AddRange(e.Data);
                if (fullMsg.Last() != 0xF7) fullMsg.Add(0xF7);

                var f0array = fullMsg.ToArray();
                var msgType = f0array.Length > 4 ? f0array[4] : (byte)0x00;

                switch(msgType)
                {
                case SysExService.COMMAND_ID_DT1:
                    var dt1Msg = _sysExService.ParseDt1Message(f0array);
                    if (!dt1Msg.IsValid)
                    {
                        Console.WriteLine($"[MIDI] DT1 received ({f0array.Length} bytes) but failed validation.");
                        Console.WriteLine($"[MIDI] Header: {BitConverter.ToString(f0array.Take(10).ToArray()).Replace("-", " ")}");

                        // Manual check for diagnostics
                        var addr = f0array.Skip(5).Take(3).ToArray();
                        var data = f0array.Skip(8).Take(f0array.Length - 10).ToArray();
                        var body = new List<byte>();
                        body.AddRange(addr);
                        body.AddRange(data);
                        var calculated = _sysExService.CalculateChecksum(body);
                        var received = f0array[f0array.Length - 2];
                        Console.WriteLine($"[MIDI] Checksum - Received: {received:X2}, Calculated: {calculated:X2}");
                        return;
                    }

                    byteBuffer.AddRange(dt1Msg.Data);
                    Console.Write($"\r[MIDI] Data received: {byteBuffer.Count} / 8192 bytes...");
                    byteProgress?.Report(totalByteOffset + byteBuffer.Count);

                    // Release once we have accumulated the expected bytes for the group
                    if (byteBuffer.Count >= 8192)
                    {
                        Console.WriteLine();
                        tcs.TrySetResult(true);
                    }
                    break;
                default:
                    Console.WriteLine($"[MIDI] Received other message type: {msgType:X2}");
                    break;
                }
            }

            _midiService.SysExReceived += SysExHandler;

            try
            {
                var progressOffset = 0;
                if (includeGroupA)
                {
                    tcs = new TaskCompletionSource<bool>();
                    byteBuffer.Clear();
                    Console.WriteLine("[MIDI] Requesting Group A (64 patches)...");
                    byte[] addressA = [0x01, 0x00, 0x00];
                    byte[] sizeA = [0x00, 0x40, 0x00];
                    await _midiService.RequestDataDump(addressA, sizeA);

                    await tcs.Task;
                    var groupABytes = byteBuffer.ToArray();
                    Console.WriteLine($"[MIDI] Group A received: {groupABytes.Length} bytes");
                    var groupAPatches = ParsePatchBuffer(groupABytes, progress, progressOffset);
                    patches.AddRange(groupAPatches);
                    progressOffset += groupAPatches.Count;
                    totalByteOffset += groupABytes.Length;
                }

                if (includeGroupB)
                {
                    tcs = new TaskCompletionSource<bool>();
                    byteBuffer.Clear();
                    Console.WriteLine("[MIDI] Requesting Group B (64 patches)...");
                    byte[] addressB = [0x02, 0x00, 0x00];
                    byte[] sizeB = [0x00, 0x40, 0x00];
                    await _midiService.RequestDataDump(addressB, sizeB);

                    await tcs.Task;
                    var groupBBytes = byteBuffer.ToArray();
                    Console.WriteLine($"[MIDI] Group B received: {groupBBytes.Length} bytes");
                    var groupBPatches = ParsePatchBuffer(groupBBytes, progress, progressOffset);
                    patches.AddRange(groupBPatches);
                }
            }
            finally
            {
                _midiService.SysExReceived -= SysExHandler;
            }

            return patches;
        }

        private static List<Patch> ParsePatchBuffer(byte[] buffer, IProgress<int>? progress, int progressOffset = 0)
        {
            var patches = new List<Patch>();
            int totalPatches = buffer.Length / PATCH_SIZE;
            Debug.WriteLine($"[DEBUG] Parsing {totalPatches} patches from buffer of size {buffer.Length} bytes");
            Debug.WriteLine(HexDump(buffer));

            for (int i = 0; i < totalPatches; i++)
            {
                var patchBytes = buffer.Skip(i * PATCH_OFFSET).Take(PATCH_SIZE).ToArray();
                //Debug.WriteLine($"[DEBUG] Parsing patch {i + 1}");
                //Debug.WriteLine(HexDump(patchBytes));
                var patch = new Patch();
                patch.ParsePatchData(patchBytes);
                patches.Add(patch);
                Debug.WriteLine($"[DEBUG] Parsed patch {i + 1}: {patch.PatchName}");
                progress?.Report(progressOffset + i + 1);
            }

            return patches;
        }
    }
}
