using System.Collections.ObjectModel;
using System.Text;

namespace GP16Editor.Cli.Models
{
    public class Patch : NotifyPropertyChangedBase
    {
        private string _patchName = "";
        public string PatchName { get => _patchName; set => SetProperty(ref _patchName, value, nameof(PatchName)); }

        public ObservableCollection<int> BlockA { get; } = new ObservableCollection<int>();
        public ObservableCollection<int> BlockB { get; } = new ObservableCollection<int>();

        public CompressorParameters Compressor { get; } = new CompressorParameters();
        public DistortionOverdriveParameters DistortionOverdrive { get; } = new DistortionOverdriveParameters();
        public PickingFilterParameters PickingFilter { get; } = new PickingFilterParameters();
        public StepPhaserParameters StepPhaser { get; } = new StepPhaserParameters();
        public ParametricEQParameters ParametricEQ { get; } = new ParametricEQParameters();
        public NoiseSuppressorParameters NoiseSuppressor { get; } = new NoiseSuppressorParameters();
        public ShortDelayParameters ShortDelay { get; } = new ShortDelayParameters();
        public ChorusParameters Chorus { get; } = new ChorusParameters();
        public FlangerParameters Flanger { get; } = new FlangerParameters();
        public PitchShifterParameters PitchShifter { get; } = new PitchShifterParameters();
        public SpaceDParameters SpaceD { get; } = new SpaceDParameters();
        public AutoPanpotParameters AutoPanpot { get; } = new AutoPanpotParameters();
        public TapDelayParameters TapDelay { get; } = new TapDelayParameters();
        public ReverbParameters Reverb { get; } = new ReverbParameters();
        public LineoutFilterParameters LineoutFilter { get; } = new LineoutFilterParameters();

        public void Parse(byte[] sysex)
        {
            // Expected SysEx format for a bulk DT1 message from GP-16:
            // F0 41 [Device_ID] 2A 12 [Address] [DATA] [Checksum] F7

            // Minimum length for a meaningful patch dump (header + address + 16-byte name + checksum + F7)
            // 70 (data) + 1 (F0) + 1 (41) + 1 (DevID) + 1 (2A) + 1 (CmdID) + 3 (Address) + 1 (Checksum) + 1 (F7) = 80 bytes.
            if (sysex.Length < 80) return; 

            // Verify header and command
            if (sysex[0] != 0xF0 || sysex[1] != 0x41 || sysex[3] != 0x2A || sysex[4] != 0x12)
            {
                // Not a Roland DT1 message or not for GP-16
                return;
            }

            // Extract data payload
            // Skip F0, 41, DeviceID, 2A, 12, Address (3 bytes).
            // Data starts at index 8 (sysex[8]).
            // The last two bytes are checksum and F7, so we ignore them.
            // The user's example has an extra F7 at the end. We'll handle it.
            var dataStart = 8; // After F0 41 DevID 2A 12 Addr1 Addr2 Addr3
            var dataLength = sysex.Length - dataStart - 2; // - checksum, - F7

            // If the message has the example's extra F7, we need to adjust
            if (sysex[sysex.Length - 1] == 0xF7 && sysex[sysex.Length - 2] == 0xF7)
            {
                dataLength = sysex.Length - dataStart - 3; // - checksum, - F7, - extra F7
            }
            
            // Ensure dataLength is not negative
            if (dataLength < 0) return;

            var data = new byte[dataLength];
            Array.Copy(sysex, dataStart, data, 0, dataLength);

            // Per GEMINI.md 7.3: Patch Name is at Address 00 00 00 through 00 00 0F (16 bytes)
            if (data.Length >= 16)
            {
                PatchName = Encoding.ASCII.GetString(data, 0x00, 16).TrimEnd('\0', ' ');
            }

            // Parse parameters using offsets from GEMINI.md
            // Compressor
            if (data.Length > 0x07)
            {
                Compressor.Sustain = data[0x06];
                Compressor.Attack = data[0x07];
            }

            // Distortion/Overdrive
            if (data.Length > 0x0C)
            {
                DistortionOverdrive.Drive = data[0x0B];
                DistortionOverdrive.Turbo = data[0x0C] == 1;
            }

            // Picking Filter
            if (data.Length > 0x13)
            {
                PickingFilter.CutoffFrequency = data[0x11];
                PickingFilter.IsUp = data[0x13] == 1;
            }

            // Chorus
            if (data.Length > 0x25)
            {
                Chorus.PreDelay = data[0x23];
                Chorus.Rate = data[0x24];
                Chorus.Depth = data[0x25];
            }

            // Reverb
            if (data.Length > 0x3F)
            {
                Reverb.Time = data[0x3D];
                Reverb.Type = data[0x3F];
            }
            
            // BlockA and BlockB (Effect Chain) are not explicitly mapped in GEMINI.md.
            // Leaving BlockA and BlockB empty for now.
            // NOTE: The GEMINI.md file provides a partial map. Other parameters and effect chain are not parsed yet.
        }
    }
}
