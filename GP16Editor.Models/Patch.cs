using System.Collections.ObjectModel;
using System.Diagnostics;
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

        public int PatchNumber { get; set; }

        public void ParsePatchData(byte[] data)
        {
            var verifiable = data[0] == 1;
            var bulkDumpType = data[1];
            var tempMemory = data[2] == 1;
            PatchNumber = data[3];

            // Patch Name (0x64-0x73, but also often at 0x00-0x0F in temp buffer)
            if (data.Length >= 0x64 + 16)
                PatchName = Encoding.ASCII.GetString(data, 0x63, 16).TrimEnd('\0', ' ');

            // BlockA and BlockB (effect order, 0x00-0x0A)
            BlockA.Clear();
            BlockB.Clear();
            for (int i = 0; i <= 4 && i < data.Length; i++) BlockA.Add(data[i]);
            for (int i = 6; i <= 10 && i < data.Length; i++) BlockB.Add(data[i]);

            // Compressor (A-1, 0x0F-0x12)
            if (data.Length > 0x12)
            {
                Compressor.Tone = (sbyte)data[0x0F] - 50;
                Compressor.Attack = data[0x10];
                Compressor.Sustain = data[0x11];
                Compressor.Level = data[0x12];
            }

            // Distortion/Overdrive (A-2, 0x13-0x16)
            if (data.Length > 0x16)
            {
                DistortionOverdrive.Tone = (sbyte)data[0x13] - 50;
                DistortionOverdrive.Drive = data[0x14];
                DistortionOverdrive.Turbo = data[0x15] != 0;
                DistortionOverdrive.Level = data[0x16];
            }

            // Picking Filter (A-3, 0x1A-0x1D)
            if (data.Length > 0x1D)
            {
                PickingFilter.Sens = data[0x1A];
                PickingFilter.CutoffFrequency = data[0x1B];
                PickingFilter.Q = data[0x1C];
                PickingFilter.IsUp = (data[0x1D] & 0x01) != 0;
            }

            // Step Phaser (A-4, 0x1E-0x22)
            if (data.Length > 0x22)
            {
                StepPhaser.Rate = data[0x1E];
                StepPhaser.Depth = data[0x1F];
                StepPhaser.Manual = data[0x20];
                StepPhaser.Resonance = data[0x21];
                StepPhaser.LfoStep = data[0x22];
            }

            // Parametric EQ (A-5, 0x23-0x2D)
            if (data.Length > 0x2D)
            {
                ParametricEQ.HiFreq = data[0x23];
                ParametricEQ.HiLevel = data[0x24] - 12;
                ParametricEQ.HMFreq = data[0x25];
                ParametricEQ.HMQ = data[0x26];
                ParametricEQ.HMLevel = data[0x27] - 12;
                ParametricEQ.LMFreq = data[0x28];
                ParametricEQ.LMQ = data[0x29];
                ParametricEQ.LMLevel = data[0x2A] - 12;
                ParametricEQ.LoFreq = data[0x2B];
                ParametricEQ.LoLevel = data[0x2C] - 12;
                ParametricEQ.OutLevel = data[0x2D] - 12;
            }

            // Noise Suppressor (A-6, 0x2E-0x30)
            if (data.Length > 0x30)
            {
                NoiseSuppressor.Sens = data[0x2E];
                NoiseSuppressor.Release = data[0x2F];
                NoiseSuppressor.Level = data[0x30];
            }

            // Short Delay (B-1, 0x31-0x32)
            if (data.Length > 0x32)
            {
                ShortDelay.DelayTime = data[0x31];
                ShortDelay.EffectLevel = data[0x32];
            }

            // Chorus (B-2a, 0x33-0x36)
            if (data.Length > 0x36)
            {
                Chorus.PreDelay = data[0x33];
                Chorus.Rate = data[0x34];
                Chorus.Depth = data[0x35];
                Chorus.EffectLevel = data[0x36];
            }

            // Flanger (B-2b, 0x37-0x3A)
            if (data.Length > 0x3A)
            {
                Flanger.Rate = data[0x37];
                Flanger.Depth = data[0x38];
                Flanger.Manual = data[0x39];
                Flanger.Resonance = data[0x3A];
            }

            // Pitch Shifter (B-2c, 0x3B-0x40)
            if (data.Length > 0x40)
            {
                PitchShifter.Balance = data[0x3B];
                PitchShifter.BalanceLSB = data[0x3C];
                PitchShifter.Chromatic = (sbyte)data[0x3D] - 12;
                PitchShifter.Fine = (sbyte)data[0x3E] - 50;
                PitchShifter.Feedback = data[0x3F];
                PitchShifter.PreDelay = data[0x40];
            }

            // Space D (B-2d, 0x41)
            if (data.Length > 0x41)
                SpaceD.Mode = data[0x41];

            // Auto Panpot (B-3, 0x42-0x44)
            if (data.Length > 0x44)
            {
                AutoPanpot.Rate = data[0x42];
                AutoPanpot.Depth = data[0x43];
                AutoPanpot.Mode = data[0x44];
            }

            // Tap Delay (B-4, 0x45-0x4E)
            if (data.Length > 0x4E)
            {
                TapDelay.CTapMSB = data[0x45];
                TapDelay.CTapLSB = data[0x46];
                TapDelay.LTapMSB = data[0x47];
                TapDelay.LTapLSB = data[0x48];
                TapDelay.RTapMSB = data[0x49];
                TapDelay.RTapLSB = data[0x4A];
                TapDelay.CLevel = data[0x4B];
                TapDelay.LLevel = data[0x4C];
                TapDelay.RLevel = data[0x4D];
                TapDelay.Feedback = data[0x4E];
            }

            // Tap Delay Cutoff (0x4F-0x50)
            if (data.Length > 0x50)
            {
                TapDelay.CutoffMSB = data[0x4F];
                TapDelay.CutoffLSB = data[0x50];
            }

            // Reverb (B-5, 0x51-0x56)
            if (data.Length > 0x56)
            {
                Reverb.Decay = data[0x51];
                Reverb.Mode = data[0x52];
                Reverb.CutoffMSB = data[0x53];
                Reverb.CutoffLSB = data[0x54];
                Reverb.PreDelay = data[0x55];
                Reverb.EffectLevel = data[0x56];
            }

            // Lineout Filter (B-6, 0x57-0x5A)
            if (data.Length > 0x5A)
            {
                LineoutFilter.Presence = data[0x57];
                LineoutFilter.Treble = data[0x58];
                LineoutFilter.Level = data[0x59];
                LineoutFilter.Bass = data[0x5A];
            }

            // Master Volume (0x5B)
            if (data.Length > 0x5B)
                LineoutFilter.MasterVolume = data[0x5B];

            // Output Channel (0x63)
            if (data.Length > 0x63)
                LineoutFilter.OutputChannel = data[0x63];

            // Patch Name (again, 0x64-0x73)
            if (data.Length > 0x73)
                PatchName = Encoding.ASCII.GetString(data, 0x64, 16).TrimEnd('\0', ' ');
        }
    }
}
