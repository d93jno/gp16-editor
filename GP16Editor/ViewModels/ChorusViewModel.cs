using System.ComponentModel;

namespace GP16Editor.ViewModels
{
    public class ChorusViewModel : INotifyPropertyChanged
    {
        private int _preDelay;
        public int PreDelay
        {
            get => _preDelay;
            set
            {
                if (_preDelay != value)
                {
                    _preDelay = value;
                    OnPropertyChanged(nameof(PreDelay));
                }
            }
        }

        private int _rate;
        public int Rate
        {
            get => _rate;
            set
            {
                if (_rate != value)
                {
                    _rate = value;
                    OnPropertyChanged(nameof(Rate));
                }
            }
        }

        private int _depth;
        public int Depth
        {
            get => _depth;
            set
            {
                if (_depth != value)
                {
                    _depth = value;
                    OnPropertyChanged(nameof(Depth));
                }
            }
        }

        private int _effectLevel;
        public int EffectLevel
        {
            get => _effectLevel;
            set
            {
                if (_effectLevel != value)
                {
                    _effectLevel = value;
                    OnPropertyChanged(nameof(EffectLevel));
                }
            }
        }

        public event PropertyChangedEventHandler? PropertyChanged;

        protected virtual void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}