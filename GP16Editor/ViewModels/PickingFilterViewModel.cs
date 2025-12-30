using System.ComponentModel;

namespace GP16Editor.ViewModels
{
    public class PickingFilterViewModel : INotifyPropertyChanged
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

        private int _cutoffFrequency;
        public int CutoffFrequency
        {
            get => _cutoffFrequency;
            set
            {
                if (_cutoffFrequency != value)
                {
                    _cutoffFrequency = value;
                    OnPropertyChanged(nameof(CutoffFrequency));
                }
            }
        }

        private double _qControl;
        public double QControl
        {
            get => _qControl;
            set
            {
                if (_qControl != value)
                {
                    _qControl = value;
                    OnPropertyChanged(nameof(QControl));
                }
            }
        }

        private string? _upDown;
        public string? UpDown
        {
            get => _upDown;
            set
            {
                if (_upDown != value)
                {
                    _upDown = value;
                    OnPropertyChanged(nameof(UpDown));
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