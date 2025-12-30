using System.ComponentModel;

namespace GP16Editor.ViewModels
{
    public class FlangerViewModel : INotifyPropertyChanged
    {
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

        private int _manual;
        public int Manual
        {
            get => _manual;
            set
            {
                if (_manual != value)
                {
                    _manual = value;
                    OnPropertyChanged(nameof(Manual));
                }
            }
        }

        private int _resonance;
        public int Resonance
        {
            get => _resonance;
            set
            {
                if (_resonance != value)
                {
                    _resonance = value;
                    OnPropertyChanged(nameof(Resonance));
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