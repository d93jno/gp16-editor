using System.ComponentModel;

namespace GP16Editor.ViewModels
{
    public class DistortionOverdriveViewModel : INotifyPropertyChanged
    {
        private int _drive;
        public int Drive
        {
            get => _drive;
            set
            {
                if (_drive != value)
                {
                    _drive = value;
                    OnPropertyChanged(nameof(Drive));
                }
            }
        }

        private bool _turbo;
        public bool Turbo
        {
            get => _turbo;
            set
            {
                if (_turbo != value)
                {
                    _turbo = value;
                    OnPropertyChanged(nameof(Turbo));
                }
            }
        }

        private int _tone;
        public int Tone
        {
            get => _tone;
            set
            {
                if (_tone != value)
                {
                    _tone = value;
                    OnPropertyChanged(nameof(Tone));
                }
            }
        }

        private int _level;
        public int Level
        {
            get => _level;
            set
            {
                if (_level != value)
                {
                    _level = value;
                    OnPropertyChanged(nameof(Level));
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