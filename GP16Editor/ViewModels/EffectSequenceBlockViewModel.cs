using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.ComponentModel;
using GP16Editor.Models;

namespace GP16Editor.ViewModels
{
    public class EffectSequenceBlockViewModel : INotifyPropertyChanged
    {
        public ObservableCollection<EffectSequenceItem> Effects { get; } = new();

        private string _blockName = string.Empty;
        public string BlockName
        {
            get => _blockName;
            set
            {
                if (_blockName != value)
                {
                    _blockName = value;
                    OnPropertyChanged(nameof(BlockName));
                }
            }
        }

        private string _sequenceString = string.Empty;
        public string SequenceString
        {
            get => _sequenceString;
            private set
            {
                if (_sequenceString != value)
                {
                    _sequenceString = value;
                    OnPropertyChanged(nameof(SequenceString));
                }
            }
        }

        private string _activeString = string.Empty;
        public string ActiveString
        {
            get => _activeString;
            private set
            {
                if (_activeString != value)
                {
                    _activeString = value;
                    OnPropertyChanged(nameof(ActiveString));
                }
            }
        }

        public EffectSequenceBlockViewModel(string blockName, IEnumerable<EffectSequenceItem> initialEffects)
        {
            BlockName = blockName;
            foreach (var effect in initialEffects)
            {
                Effects.Add(effect);
            }
            UpdateStrings();

            Effects.CollectionChanged += OnEffectsChanged;
            foreach (var effect in Effects)
            {
                effect.PropertyChanged += OnEffectPropertyChanged;
            }
        }

        private void OnEffectsChanged(object? sender, NotifyCollectionChangedEventArgs e)
        {
            if (e.NewItems != null)
            {
                foreach (EffectSequenceItem item in e.NewItems)
                {
                    item.PropertyChanged += OnEffectPropertyChanged;
                }
            }
            if (e.OldItems != null)
            {
                foreach (EffectSequenceItem item in e.OldItems)
                {
                    item.PropertyChanged -= OnEffectPropertyChanged;
                }
            }
            UpdateOrders();
            UpdateStrings();
        }

        private void OnEffectPropertyChanged(object? sender, PropertyChangedEventArgs e)
        {
            if (e.PropertyName == nameof(EffectSequenceItem.IsEnabled))
            {
                UpdateStrings();
            }
        }

        private void UpdateOrders()
        {
            for (int i = 0; i < Effects.Count; i++)
            {
                Effects[i].Order = i;
            }
        }

        private void UpdateStrings()
        {
            SequenceString = "Sequence: " + string.Join(",", Effects.OrderBy(e => e.Order).Select(e => e.Id));
            var active = Effects.Where(e => e.IsEnabled).OrderBy(e => e.Order).Select(e => e.Id);
            ActiveString = "Active: *" + string.Join("**", active) + "*";
        }

        public event PropertyChangedEventHandler? PropertyChanged;

        protected virtual void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}