using System.ComponentModel;

namespace GP16Editor.ViewModels
{
    public class PitchShifterViewModel : INotifyPropertyChanged
    {
        private int _chromatic;
        public int Chromatic
        {
            get => _chromatic;
            set
            {
                if (_chromatic != value)
                {
                    _chromatic = value;
                    OnPropertyChanged(nameof(Chromatic));
                }
            }
        }

        private int _fine;
        public int Fine
        {
            get => _fine;
            set
            {
                if (_fine != value)
                {
                    _fine = value;
                    OnPropertyChanged(nameof(Fine));
                }
            }
        }

        private int _balance;
        public int Balance
        {
            get => _balance;
            set
            {
                if (_balance != value)
                {
                    _balance = value;
                    OnPropertyChanged(nameof(Balance));
                }
            }
        }

        private int _feedback;
        public int Feedback
        {
            get => _feedback;
            set
            {
                if (_feedback != value)
                {
                    _feedback = value;
                    OnPropertyChanged(nameof(Feedback));
                }
            }
        }

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

        public event PropertyChangedEventHandler? PropertyChanged;

        protected virtual void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}