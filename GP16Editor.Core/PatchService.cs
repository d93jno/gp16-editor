using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Threading.Tasks;
using GP16Editor.Models;
using Melanchall.DryWetMidi.Core;

namespace GP16Editor.Core
{
    public class PatchService
    {
        private readonly MidiService _midiService;
        private readonly SysExService _sysExService;

        private const int PATCH_SIZE = 70;
        private const int PATCHES_PER_GROUP = 64;
        private const int TOTAL_PATCHES = 128;
        private const int GROUP_A_PATCH_COUNT = 64;
        private const int GROUP_B_PATCH_COUNT = 64;
        private const int GROUP_A_SIZE = PATCH_SIZE * GROUP_A_PATCH_COUNT; // 4480
        private const int GROUP_B_SIZE = PATCH_SIZE * GROUP_B_PATCH_COUNT; // 4480
        private const int TOTAL_DATA_SIZE = GROUP_A_SIZE + GROUP_B_SIZE; // 8960
        
        public PatchService(MidiService midiService, SysExService sysExService)
        {
            _midiService = midiService;
            _sysExService = sysExService;
        }

        public async Task<List<Patch>> GetAllPatchesAsync()
        {
            var patches = new List<Patch>();
            var incomingData = new List<byte>();
            var tcs = new TaskCompletionSource<bool>();

            void SysExHandler(object? sender, NormalSysExEvent e)
            {
                // The data from NormalSysExEvent does not include F0 and F7.
                // The actual payload is in e.Data. We need to check if this is a DT1 message.
                // A DT1 message from GP-16 will start with: 41 dev 2A 12 ...
                // But e.Data will be after the 41, so it will start with dev 2A 12 ...
                // Let's assume the deviceId matches.
                
                if (e.Data.Length > 3 && e.Data[0] == _midiService.DeviceId && e.Data[1] == 0x2A && e.Data[2] == 0x12)
                {
                    // This is a DT1 message. The data follows the address.
                    // The address is 3 bytes, so data starts at index 3+3=6 in the sysex message payload (e.Data)
                    // The address bytes are e.Data[3], e.Data[4], e.Data[5]
                    // The actual patch data starts after the address
                    var patchData = e.Data.Skip(6).ToArray();
                    incomingData.AddRange(patchData);

                    if (incomingData.Count >= TOTAL_DATA_SIZE)
                    {
                        tcs.TrySetResult(true);
                    }
                }
            }

            _midiService.SysExReceived += SysExHandler;

            try
            {
                // Request Group A
                byte[] addressA = { 0x01, 0x00, 0x00 };
                byte[] sizeA = { 0x00, 0x23, 0x00 }; // 4480 bytes
                await _midiService.RequestDataDump(addressA, sizeA);

                // Request Group B
                byte[] addressB = { 0x02, 0x00, 0x00 };
                byte[] sizeB = { 0x00, 0x23, 0x00 }; // 4480 bytes
                await _midiService.RequestDataDump(addressB, sizeB);

                // Wait for data to arrive, with a timeout.
                var timeoutTask = Task.Delay(5000);
                var completedTask = await Task.WhenAny(tcs.Task, timeoutTask);

                if (completedTask == tcs.Task)
                {
                    // Data received. Now parse it.
                    for (int i = 0; i < TOTAL_PATCHES; i++)
                    {
                        var patchData = incomingData.Skip(i * PATCH_SIZE).Take(PATCH_SIZE).ToArray();
                        if(patchData.Length == PATCH_SIZE)
                        {
                            var patch = new Patch();
                            patch.ParsePatchData(patchData); 
                            patches.Add(patch);
                        }
                    }
                }
                else
                {
                    // Timeout
                    Debug.WriteLine("Timeout waiting for patch data.");
                }
            }
            finally
            {
                _midiService.SysExReceived -= SysExHandler;
            }

            return patches;
        }
    }
}