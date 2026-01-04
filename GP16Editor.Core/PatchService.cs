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

        public async Task<List<Patch>> GetAllPatchesAsync(IProgress<int>? progress)
        {
            var patches = new List<Patch>();
            var tcs = new TaskCompletionSource<bool>();
            var byteBuffer = new List<byte>();

            void SysExHandler(object? sender, NormalSysExEvent e)
            {
                var msgType = e.Data.Length > 3 ? e.Data[3] : (byte)0x00;

                switch(msgType)
                {
                case SysExService.COMMAND_ID_DT1:
                    var address = (e.Data[4] << 32) | (e.Data[5] << 16) | e.Data[6] << 8 | e.Data[7];
                    Debug.WriteLine($"[DEBUG] SysEx DT1 message received {e.Data.Length} bytes, address {address:X8}");
                    //Debug.WriteLine(HexDump(e.Data));
                    var patchData = e.Data.Skip(8).Take(e.Data.Length - 10).ToArray();
                    byteBuffer.AddRange(patchData);
                    Debug.WriteLine("[DEBUG] Current patch buffer size: " + patchData.Length);
                    //Debug.WriteLine(HexDump(patchData));

                    // Release it once we have all data per group
                    if (e.Data.Length < 254)
                    {
                        tcs.TrySetResult(true);
                    }
                    break;
                default:
                    Debug.WriteLine($"[DEBUG] Unknown SysEx message type received: {msgType:X2}");
                    break;
                }
            }

            _midiService.SysExReceived += SysExHandler;

            try
            {
                // Request Group A
                Debug.WriteLine("[DEBUG] Requesting Group A data dump");
                byte[] addressA = [0x01, 0x00, 0x00];
                byte[] sizeA = [0x00, 0x23, 0x00]; // 4480 bytes
                await _midiService.RequestDataDump(addressA, sizeA);

                await tcs.Task;
                var groupABytes = byteBuffer.ToArray();
                Debug.WriteLine("[DEBUG] Group A data received");
                //Debug.WriteLine(HexDump(groupABytes));
                var groupAPatches = ParsePatchBuffer(groupABytes, progress);

                // Request Group B
                tcs = new TaskCompletionSource<bool>();
                byteBuffer.Clear();
                Debug.WriteLine("[DEBUG] Requesting Group B data dump");
                byte[] addressB = [0x02, 0x00, 0x00];
                byte[] sizeB = [0x00, 0x23, 0x00]; // 4480 bytes
                await _midiService.RequestDataDump(addressB, sizeB);

                await tcs.Task;
                Debug.WriteLine("[DEBUG] Group B data received");
                var groupBBytes = byteBuffer.ToArray();
                var groupBPatches = ParsePatchBuffer(groupBBytes, progress);
            }
            finally
            {
                _midiService.SysExReceived -= SysExHandler;
            }

            return patches;
        }

        private static List<Patch> ParsePatchBuffer(byte[] buffer, IProgress<int>? progress)
        {
            var patches = new List<Patch>();
            int totalPatches = buffer.Length / PATCH_SIZE;
            Debug.WriteLine($"[DEBUG] Parsing {totalPatches} patches from buffer of size {buffer.Length} bytes");
            Debug.WriteLine(HexDump(buffer));

            for (int i = 0; i < totalPatches; i++)
            {
                var patchBytes = buffer.Skip(i * PATCH_OFFSET).Take(PATCH_SIZE).ToArray();
                Debug.WriteLine($"[DEBUG] Parsing patch {i + 1}");
                Debug.WriteLine(HexDump(patchBytes));
                var patch = new Patch();
                patch.ParsePatchData(patchBytes);
                patches.Add(patch);
                Debug.WriteLine($"[DEBUG] Parsed patch {i + 1}: {patch.PatchName}");
                progress?.Report(i + 1);
            }

            return patches;
        }
    }
}