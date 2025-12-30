using System.ComponentModel;

namespace GP16Editor.ViewModels
{
    public class TapDelayViewModel : INotifyPropertyChanged
    {
        private int _centerTapTime;
        public int CenterTapTime { get => _centerTapTime; set { _centerTapTime = value; OnPropertyChanged(nameof(CenterTapTime)); } }

        private int _centerTapLevel;
        public int CenterTapLevel { get => _centerTapLevel; set { _centerTapLevel = value; OnPropertyChanged(nameof(CenterTapLevel)); } }

        private int _leftTapTime;
        public int LeftTapTime { get => _leftTapTime; set { _leftTapTime = value; OnPropertyChanged(nameof(LeftTapTime)); } }

        private int _leftTapLevel;
        public int LeftTapLevel { get => _leftTapLevel; set { _leftTapLevel = value; OnPropertyChanged(nameof(LeftTapLevel)); } }

        private int _rightTapTime;
        public int RightTapTime { get => _rightTapTime; set { _rightTapTime = value; OnPropertyChanged(nameof(RightTapTime)); } }

        private int _rightTapLevel;
        public int RightTapLevel { get => _rightTapLevel; set { _rightTapLevel = value; OnPropertyChanged(nameof(RightTapLevel)); } }

        private int _feedback;
        public int Feedback { get => _feedback; set { _feedback = value; OnPropertyChanged(nameof(Feedback)); } }

        private string? _cutoff;
        public string? Cutoff { get => _cutoff; set { _cutoff = value; OnPropertyChanged(nameof(Cutoff)); } }

        public event PropertyChangedEventHandler? PropertyChanged;

        protected virtual void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}