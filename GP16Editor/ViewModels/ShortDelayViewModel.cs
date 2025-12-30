using System.ComponentModel;

namespace GP16Editor.ViewModels
{
    public class ShortDelayViewModel : INotifyPropertyChanged
    {
        private int _delayTime;
        public int DelayTime
        {
            get => _delayTime;
            set
            {
                if (_delayTime != value)
                {
                    _delayTime = value;
                    OnPropertyChanged(nameof(DelayTime));
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