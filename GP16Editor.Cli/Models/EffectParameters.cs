namespace GP16Editor.Cli.Models
{
    public class CompressorParameters : NotifyPropertyChangedBase
    {
        private int _sustain;
        public int Sustain { get => _sustain; set => SetProperty(ref _sustain, value, nameof(Sustain)); }

        private int _attack;
        public int Attack { get => _attack; set => SetProperty(ref _attack, value, nameof(Attack)); }

        private int _tone;
        public int Tone { get => _tone; set => SetProperty(ref _tone, value, nameof(Tone)); }

        private int _level;
        public int Level { get => _level; set => SetProperty(ref _level, value, nameof(Level)); }
    }

    public class DistortionOverdriveParameters : NotifyPropertyChangedBase
    {
        private bool _isDistortion;
        public bool IsDistortion { get => _isDistortion; set => SetProperty(ref _isDistortion, value, nameof(IsDistortion)); }

        private int _drive;
        public int Drive { get => _drive; set => SetProperty(ref _drive, value, nameof(Drive)); }

        private bool _turbo;
        public bool Turbo { get => _turbo; set => SetProperty(ref _turbo, value, nameof(Turbo)); }

        private int _tone;
        public int Tone { get => _tone; set => SetProperty(ref _tone, value, nameof(Tone)); }

        private int _level;
        public int Level { get => _level; set => SetProperty(ref _level, value, nameof(Level)); }
    }

    public class PickingFilterParameters : NotifyPropertyChangedBase
    {
        private int _sensitivity;
        public int Sensitivity { get => _sensitivity; set => SetProperty(ref _sensitivity, value, nameof(Sensitivity)); }

        private int _cutoffFrequency;
        public int CutoffFrequency { get => _cutoffFrequency; set => SetProperty(ref _cutoffFrequency, value, nameof(CutoffFrequency)); }

        private double _q;
        public double Q { get => _q; set => SetProperty(ref _q, value, nameof(Q)); }

        private bool _isUp;
        public bool IsUp { get => _isUp; set => SetProperty(ref _isUp, value, nameof(IsUp)); }
    }

    public class StepPhaserParameters : NotifyPropertyChangedBase
    {
        private int _rate;
        public int Rate { get => _rate; set => SetProperty(ref _rate, value, nameof(Rate)); }

        private int _depth;
        public int Depth { get => _depth; set => SetProperty(ref _depth, value, nameof(Depth)); }
        
        private int _manual;
        public int Manual { get => _manual; set => SetProperty(ref _manual, value, nameof(Manual)); }

        private int _resonance;
        public int Resonance { get => _resonance; set => SetProperty(ref _resonance, value, nameof(Resonance)); }

        private int _stepRate;
        public int StepRate { get => _stepRate; set => SetProperty(ref _stepRate, value, nameof(StepRate)); }
    }

    public class ParametricEQParameters : NotifyPropertyChangedBase
    {
        private int _lowGain;
        public int LowGain { get => _lowGain; set => SetProperty(ref _lowGain, value, nameof(LowGain)); }
        
        private int _highGain;
        public int HighGain { get => _highGain; set => SetProperty(ref _highGain, value, nameof(HighGain)); }

        private int _midFreq;
        public int MidFreq { get => _midFreq; set => SetProperty(ref _midFreq, value, nameof(MidFreq)); }
        
        private int _midQ;
        public int MidQ { get => _midQ; set => SetProperty(ref _midQ, value, nameof(MidQ)); }
        
        private int _midGain;
        public int MidGain { get => _midGain; set => SetProperty(ref _midGain, value, nameof(MidGain)); }
    }

    public class NoiseSuppressorParameters : NotifyPropertyChangedBase
    {
        private int _threshold;
        public int Threshold { get => _threshold; set => SetProperty(ref _threshold, value, nameof(Threshold)); }

        private int _release;
        public int Release { get => _release; set => SetProperty(ref _release, value, nameof(Release)); }
    }

    public class ShortDelayParameters : NotifyPropertyChangedBase
    {
        private int _delayTime;
        public int DelayTime { get => _delayTime; set => SetProperty(ref _delayTime, value, nameof(DelayTime)); }

        private int _feedback;
        public int Feedback { get => _feedback; set => SetProperty(ref _feedback, value, nameof(Feedback)); }
        
        private int _modRate;
        public int ModRate { get => _modRate; set => SetProperty(ref _modRate, value, nameof(ModRate)); }
        
        private int _modDepth;
        public int ModDepth { get => _modDepth; set => SetProperty(ref _modDepth, value, nameof(ModDepth)); }
    }

    public class ChorusParameters : NotifyPropertyChangedBase
    {
        private int _rate;
        public int Rate { get => _rate; set => SetProperty(ref _rate, value, nameof(Rate)); }
        
        private int _depth;
        public int Depth { get => _depth; set => SetProperty(ref _depth, value, nameof(Depth)); }

        private int _preDelay;
        public int PreDelay { get => _preDelay; set => SetProperty(ref _preDelay, value, nameof(PreDelay)); }
        
        private int _feedback;
        public int Feedback { get => _feedback; set => SetProperty(ref _feedback, value, nameof(Feedback)); }
    }

    public class FlangerParameters : NotifyPropertyChangedBase
    {
        private int _rate;
        public int Rate { get => _rate; set => SetProperty(ref _rate, value, nameof(Rate)); }
        
        private int _depth;
        public int Depth { get => _depth; set => SetProperty(ref _depth, value, nameof(Depth)); }

        private int _manual;
        public int Manual { get => _manual; set => SetProperty(ref _manual, value, nameof(Manual)); }
        
        private int _resonance;
        public int Resonance { get => _resonance; set => SetProperty(ref _resonance, value, nameof(Resonance)); }
    }

    public class PitchShifterParameters : NotifyPropertyChangedBase
    {
        private int _coarse;
        public int Coarse { get => _coarse; set => SetProperty(ref _coarse, value, nameof(Coarse)); }

        private int _fine;
        public int Fine { get => _fine; set => SetProperty(ref _fine, value, nameof(Fine)); }

        private int _preDelay;
        public int PreDelay { get => _preDelay; set => SetProperty(ref _preDelay, value, nameof(PreDelay)); }

        private int _feedback;
        public int Feedback { get => _feedback; set => SetProperty(ref _feedback, value, nameof(Feedback)); }
    }

    public class SpaceDParameters : NotifyPropertyChangedBase
    {
        private int _mode;
        public int Mode { get => _mode; set => SetProperty(ref _mode, value, nameof(Mode)); }
    }

    public class AutoPanpotParameters : NotifyPropertyChangedBase
    {
        private int _rate;
        public int Rate { get => _rate; set => SetProperty(ref _rate, value, nameof(Rate)); }
        
        private int _depth;
        public int Depth { get => _depth; set => SetProperty(ref _depth, value, nameof(Depth)); }

        private int _waveform;
        public int Waveform { get => _waveform; set => SetProperty(ref _waveform, value, nameof(Waveform)); }
    }

    public class TapDelayParameters : NotifyPropertyChangedBase
    {
        private int _delayTimeL;
        public int DelayTimeL { get => _delayTimeL; set => SetProperty(ref _delayTimeL, value, nameof(DelayTimeL)); }
        
        private int _delayTimeR;
        public int DelayTimeR { get => _delayTimeR; set => SetProperty(ref _delayTimeR, value, nameof(DelayTimeR)); }
        
        private int _delayTimeC;
        public int DelayTimeC { get => _delayTimeC; set => SetProperty(ref _delayTimeC, value, nameof(DelayTimeC)); }

        private int _feedback;
        public int Feedback { get => _feedback; set => SetProperty(ref _feedback, value, nameof(Feedback)); }
    }

    public class ReverbParameters : NotifyPropertyChangedBase
    {
        private int _type;
        public int Type { get => _type; set => SetProperty(ref _type, value, nameof(Type)); }

        private int _time;
        public int Time { get => _time; set => SetProperty(ref _time, value, nameof(Time)); }

        private int _preDelay;
        public int PreDelay { get => _preDelay; set => SetProperty(ref _preDelay, value, nameof(PreDelay)); }

        private int _hfDamp;
        public int HFDamp { get => _hfDamp; set => SetProperty(ref _hfDamp, value, nameof(HFDamp)); }
    }

    public class LineoutFilterParameters : NotifyPropertyChangedBase
    {
        private int _type;
        public int Type { get => _type; set => SetProperty(ref _type, value, nameof(Type)); }
    }
}
