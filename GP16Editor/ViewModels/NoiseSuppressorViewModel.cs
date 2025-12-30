using System.ComponentModel;

namespace GP16Editor.ViewModels
{
    public class NoiseSuppressorViewModel : INotifyPropertyChanged
    {
        private int _sensitivity;
        public int Sensitivity
        {
            get => _sensitivity;
            set
            {
                if (_sensitivity != value)
                {
                    _sensitivity = value;
                    OnPropertyChanged(nameof(Sensitivity));
                }
            }
        }

        private int _release;
        public int Release
        {
            get => _release;
            set
            {
                if (_release != value)
                {
                    _release = value;
                    OnPropertyChanged(nameof(Release));
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