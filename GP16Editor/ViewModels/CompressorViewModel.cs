using System.ComponentModel;

namespace GP16Editor.ViewModels
{
    public class CompressorViewModel : INotifyPropertyChanged
    {
        private int _sustain;
        public int Sustain
        {
            get => _sustain;
            set
            {
                if (_sustain != value)
                {
                    _sustain = value;
                    OnPropertyChanged(nameof(Sustain));
                }
            }
        }

        private int _attack;
        public int Attack
        {
            get => _attack;
            set
            {
                if (_attack != value)
                {
                    _attack = value;
                    OnPropertyChanged(nameof(Attack));
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
