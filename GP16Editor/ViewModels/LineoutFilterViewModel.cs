using System.ComponentModel;

namespace GP16Editor.ViewModels
{
    public class LineoutFilterViewModel : INotifyPropertyChanged
    {
        private int _presence;
        public int Presence
        {
            get => _presence;
            set
            {
                if (_presence != value)
                {
                    _presence = value;
                    OnPropertyChanged(nameof(Presence));
                }
            }
        }

        private int _treble;
        public int Treble
        {
            get => _treble;
            set
            {
                if (_treble != value)
                {
                    _treble = value;
                    OnPropertyChanged(nameof(Treble));
                }
            }
        }

        private int _middle;
        public int Middle
        {
            get => _middle;
            set
            {
                if (_middle != value)
                {
                    _middle = value;
                    OnPropertyChanged(nameof(Middle));
                }
            }
        }

        private int _bass;
        public int Bass
        {
            get => _bass;
            set
            {
                if (_bass != value)
                {
                    _bass = value;
                    OnPropertyChanged(nameof(Bass));
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