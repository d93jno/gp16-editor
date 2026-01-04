using System.ComponentModel;
using System.Collections.Generic;

namespace GP16Editor.Models
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
        private int _sens;
        public int Sens { get => _sens; set => SetProperty(ref _sens, value, nameof(Sens)); }

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

        private int _lfoStep;
        public int LfoStep { get => _lfoStep; set => SetProperty(ref _lfoStep, value, nameof(LfoStep)); }
    }

    public class ParametricEQParameters : NotifyPropertyChangedBase
    {
        private int _hiFreq;
        public int HiFreq { get => _hiFreq; set => SetProperty(ref _hiFreq, value, nameof(HiFreq)); }

        private int _hiLevel;
        public int HiLevel { get => _hiLevel; set => SetProperty(ref _hiLevel, value, nameof(HiLevel)); }

        private int _hmFreq;
        public int HMFreq { get => _hmFreq; set => SetProperty(ref _hmFreq, value, nameof(HMFreq)); }

        private int _hmQ;
        public int HMQ { get => _hmQ; set => SetProperty(ref _hmQ, value, nameof(HMQ)); }

        private int _hmLevel;
        public int HMLevel { get => _hmLevel; set => SetProperty(ref _hmLevel, value, nameof(HMLevel)); }

        private int _lmFreq;
        public int LMFreq { get => _lmFreq; set => SetProperty(ref _lmFreq, value, nameof(LMFreq)); }

        private int _lmQ;
        public int LMQ { get => _lmQ; set => SetProperty(ref _lmQ, value, nameof(LMQ)); }

        private int _lmLevel;
        public int LMLevel { get => _lmLevel; set => SetProperty(ref _lmLevel, value, nameof(LMLevel)); }

        private int _loFreq;
        public int LoFreq { get => _loFreq; set => SetProperty(ref _loFreq, value, nameof(LoFreq)); }

        private int _loLevel;
        public int LoLevel { get => _loLevel; set => SetProperty(ref _loLevel, value, nameof(LoLevel)); }

        private int _outLevel;
        public int OutLevel { get => _outLevel; set => SetProperty(ref _outLevel, value, nameof(OutLevel)); }
    }

    public class NoiseSuppressorParameters : NotifyPropertyChangedBase
    {
        private int _sens;
        public int Sens { get => _sens; set => SetProperty(ref _sens, value, nameof(Sens)); }

        private int _release;
        public int Release { get => _release; set => SetProperty(ref _release, value, nameof(Release)); }

        private int _level;
        public int Level { get => _level; set => SetProperty(ref _level, value, nameof(Level)); }
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

        private int _effectLevel;
        public int EffectLevel { get => _effectLevel; set => SetProperty(ref _effectLevel, value, nameof(EffectLevel)); }
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

        private int _effectLevel;
        public int EffectLevel { get => _effectLevel; set => SetProperty(ref _effectLevel, value, nameof(EffectLevel)); }
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
        private int _balance;
        public int Balance { get => _balance; set => SetProperty(ref _balance, value, nameof(Balance)); }

        private int _balanceLSB;
        public int BalanceLSB { get => _balanceLSB; set => SetProperty(ref _balanceLSB, value, nameof(BalanceLSB)); }

        private int _chromatic;
        public int Chromatic { get => _chromatic; set => SetProperty(ref _chromatic, value, nameof(Chromatic)); }

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
        private int _mode;
        public int Mode { get => _mode; set => SetProperty(ref _mode, value, nameof(Mode)); }
        
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

        private int _rLevel;
        public int RLevel { get => _rLevel; set => SetProperty(ref _rLevel, value, nameof(RLevel)); }

        private int _cLevel;
        public int CLevel { get => _cLevel; set => SetProperty(ref _cLevel, value, nameof(CLevel)); }

        private int _lLevel;
        public int LLevel { get => _lLevel; set => SetProperty(ref _lLevel, value, nameof(LLevel)); }

        private int _cutoffMSB;
        public int CutoffMSB { get => _cutoffMSB; set => SetProperty(ref _cutoffMSB, value, nameof(CutoffMSB)); }

        private int _cutoffLSB;
        public int CutoffLSB { get => _cutoffLSB; set => SetProperty(ref _cutoffLSB, value, nameof(CutoffLSB)); }

        private int _cTapMSB;
        public int CTapMSB { get => _cTapMSB; set => SetProperty(ref _cTapMSB, value, nameof(CTapMSB)); }

        private int _cTapLSB;
        public int CTapLSB { get => _cTapLSB; set => SetProperty(ref _cTapLSB, value, nameof(CTapLSB)); }

        private int _lTapMSB;
        public int LTapMSB { get => _lTapMSB; set => SetProperty(ref _lTapMSB, value, nameof(LTapMSB)); }

        private int _rTapMSB;
        public int RTapMSB { get => _rTapMSB; set => SetProperty(ref _rTapMSB, value, nameof(RTapMSB)); }

        private int _rTapLSB;
        public int RTapLSB { get => _rTapLSB; set => SetProperty(ref _rTapLSB, value, nameof(RTapLSB)); }

        private int _lTapLSB;
        public int LTapLSB { get => _lTapLSB; set => SetProperty(ref _lTapLSB, value, nameof(LTapLSB)); }
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

        private int _cutoffMSB;
        public int CutoffMSB { get => _cutoffMSB; set => SetProperty(ref _cutoffMSB, value, nameof(CutoffMSB)); }

        private int _cutoffLSB;
        public int CutoffLSB { get => _cutoffLSB; set => SetProperty(ref _cutoffLSB, value, nameof(CutoffLSB)); }

        private int _effectLevel;
        public int EffectLevel { get => _effectLevel; set => SetProperty(ref _effectLevel, value, nameof(EffectLevel)); }

        private int _decay;
        public int Decay { get => _decay; set => SetProperty(ref _decay, value, nameof(Decay)); }

        private int _mode;
        public int Mode { get => _mode; set => SetProperty(ref _mode, value, nameof(Mode)); }
    }

    public class LineoutFilterParameters : NotifyPropertyChangedBase
    {
        private int _type;
        public int Type { get => _type; set => SetProperty(ref _type, value, nameof(Type)); }

        private int _presence;
        public int Presence { get => _presence; set => SetProperty(ref _presence, value, nameof(Presence)); }

        private int _treble;
        public int Treble { get => _treble; set => SetProperty(ref _treble, value, nameof(Treble)); }

        private int _level;
        public int Level { get => _level; set => SetProperty(ref _level, value, nameof(Level)); }

        private int _bass;
        public int Bass { get => _bass; set => SetProperty(ref _bass, value, nameof(Bass)); }

        private int _masterVolume;
        public int MasterVolume { get => _masterVolume; set => SetProperty(ref _masterVolume, value, nameof(MasterVolume)); }

        private int _outputChannel;
        public int OutputChannel { get => _outputChannel; set => SetProperty(ref _outputChannel, value, nameof(OutputChannel)); }
    }
}
