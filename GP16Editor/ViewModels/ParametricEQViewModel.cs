using System.ComponentModel;

namespace GP16Editor.ViewModels
{
    public class ParametricEQViewModel : INotifyPropertyChanged
    {
        // Low Band
        private int _lowFreq;
        public int LowFreq { get => _lowFreq; set { _lowFreq = value; OnPropertyChanged(nameof(LowFreq)); } }

        private int _lowLevel;
        public int LowLevel { get => _lowLevel; set { _lowLevel = value; OnPropertyChanged(nameof(LowLevel)); } }

        // Low Mid Band
        private int _lowMidFreq;
        public int LowMidFreq { get => _lowMidFreq; set { _lowMidFreq = value; OnPropertyChanged(nameof(LowMidFreq)); } }

        private double _lowMidQ;
        public double LowMidQ { get => _lowMidQ; set { _lowMidQ = value; OnPropertyChanged(nameof(LowMidQ)); } }

        private int _lowMidLevel;
        public int LowMidLevel { get => _lowMidLevel; set { _lowMidLevel = value; OnPropertyChanged(nameof(LowMidLevel)); } }

        // High Mid Band
        private int _highMidFreq;
        public int HighMidFreq { get => _highMidFreq; set { _highMidFreq = value; OnPropertyChanged(nameof(HighMidFreq)); } }

        private double _highMidQ;
        public double HighMidQ { get => _highMidQ; set { _highMidQ = value; OnPropertyChanged(nameof(HighMidQ)); } }

        private int _highMidLevel;
        public int HighMidLevel { get => _highMidLevel; set { _highMidLevel = value; OnPropertyChanged(nameof(HighMidLevel)); } }

        // High Band
        private int _highFreq;
        public int HighFreq { get => _highFreq; set { _highFreq = value; OnPropertyChanged(nameof(HighFreq)); } }

        private int _highLevel;
        public int HighLevel { get => _highLevel; set { _highLevel = value; OnPropertyChanged(nameof(HighLevel)); } }

        public event PropertyChangedEventHandler? PropertyChanged;

        protected virtual void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}