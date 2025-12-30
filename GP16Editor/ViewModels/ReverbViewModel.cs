using System.ComponentModel;

namespace GP16Editor.ViewModels
{
    public class ReverbViewModel : INotifyPropertyChanged
    {
        private string? _mode;
        public string? Mode { get => _mode; set { _mode = value; OnPropertyChanged(nameof(Mode)); } }

        private double _decay;
        public double Decay { get => _decay; set { _decay = value; OnPropertyChanged(nameof(Decay)); } }

        private int _preDelay;
        public int PreDelay { get => _preDelay; set { _preDelay = value; OnPropertyChanged(nameof(PreDelay)); } }

        private int _effectLevel;
        public int EffectLevel { get => _effectLevel; set { _effectLevel = value; OnPropertyChanged(nameof(EffectLevel)); } }

        private int _cutoff;
        public int Cutoff { get => _cutoff; set { _cutoff = value; OnPropertyChanged(nameof(Cutoff)); } }

        public event PropertyChangedEventHandler? PropertyChanged;

        protected virtual void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}