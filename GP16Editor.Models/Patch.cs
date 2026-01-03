using System.Collections.ObjectModel;
using System.Text;

namespace GP16Editor.Models
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
            // This parsing logic is based on the GEMINI.md file and analysis of the provided SysEx message.
            // The provided sysex: F0 41 00 2A 12 0F 7F 00 00 ...
            // It is assumed that the data starts at index 9.

            if (sysex.Length < 100) return; // Not a valid patch dump

            // The data payload seems to start at index 9 of the sysex message
            var data = sysex.Skip(9).ToArray();

            // The last bytes are checksum and F7, so we ignore them. The user example has two F7s.
            var lastF7 = Array.LastIndexOf(data, (byte)0xF7);
            if (lastF7 > 0)
            {
                data = data.Take(lastF7 -1).ToArray();
            }
            
            ParsePatchData(data, 0);
        }

        public int PatchNumber { get; set; }
        public void ParsePatchData(byte[] data, int patchNumber)
        {
            PatchNumber = patchNumber;
            // Parameters are parsed based on the offsets provided in GEMINI.md
            
            // Patch Name is at the beginning of the patch data.
            if (data.Length >= 16)
            {
                PatchName = Encoding.ASCII.GetString(data, 0, 16).TrimEnd('\0', ' ');
            }

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
            
            // NOTE: The GEMINI.md file provides a partial map. Other parameters are not parsed yet.
        }
    }
}
