using System.ComponentModel;

namespace GP16Editor.Models
{
    public class Patch : INotifyPropertyChanged
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

        public event PropertyChangedEventHandler? PropertyChanged;

        protected virtual void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
