using System.Diagnostics;
using GP16Editor.Models;
using Melanchall.DryWetMidi.Core;

namespace GP16Editor.Core
{
    public class PatchService(MidiService midiService, SysExService sysExService)

    {
        private readonly MidiService _midiService = midiService;
        private readonly SysExService _sysExService = sysExService;

        private const int PATCH_SIZE = 70;
        private const int PATCHES_PER_GROUP = 64;
        private const int TOTAL_PATCHES = 128;


        public async Task<List<Patch>> GetAllPatchesAsync()
        {
            return await GetAllPatchesAsync(null);
        }

        public async Task<List<Patch>> GetAllPatchesAsync(IProgress<int>? progress)
        {
            var patches = new List<Patch>();
            var tcs = new TaskCompletionSource<bool>();

            void SysExHandler(object? sender, NormalSysExEvent e)
            {
                var msgType = e.Data.Length > 3 ? e.Data[3] : (byte)0x00;

                switch(msgType)
                {
                case SysExService.COMMAND_ID_DT1:
                    Debug.WriteLine("[DEBUG] SysEx DT1 message received");
                    var patchData = e.Data.Skip(4).ToArray();
                    Debug.WriteLine($"[DEBUG] Received SysEx data chunk: {string.Join(" ", patchData.Select(b => b.ToString("X2")))}");

                    var patch = new Patch();
                    patch.ParsePatchData(patchData, patches.Count + 1); 
                    patches.Add(patch);
                    Debug.WriteLine($"[DEBUG] Parsed patch {patches.Count}: {patch.PatchName}");

                    progress?.Report(patches.Count);

                    // Release it once we have all data per group
                    if (patches.Count == PATCHES_PER_GROUP || patches.Count == TOTAL_PATCHES)
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
                Debug.WriteLine("[DEBUG] Group A data received");

                // Request Group B
                tcs = new TaskCompletionSource<bool>();
                Debug.WriteLine("[DEBUG] Requesting Group B data dump");
                byte[] addressB = [0x02, 0x00, 0x00];
                byte[] sizeB = [0x00, 0x23, 0x00]; // 4480 bytes
                await _midiService.RequestDataDump(addressB, sizeB);

                await tcs.Task;
                Debug.WriteLine("[DEBUG] Group B data received");
            }
            finally
            {
                _midiService.SysExReceived -= SysExHandler;
            }

            return patches;
        }
    }
}