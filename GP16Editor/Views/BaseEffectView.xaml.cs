using Microsoft.Maui.Controls;

namespace GP16Editor.Views
{
    public partial class BaseEffectView : ContentView
    {
        public static readonly BindableProperty EffectNameProperty =
            BindableProperty.Create(nameof(EffectName), typeof(string), typeof(BaseEffectView), string.Empty);

        public string EffectName
        {
            get => (string)GetValue(EffectNameProperty);
            set => SetValue(EffectNameProperty, value);
        }

        public static readonly BindableProperty EffectContentProperty =
            BindableProperty.Create(nameof(EffectContent), typeof(View), typeof(BaseEffectView), propertyChanged: OnEffectContentChanged);

        public View EffectContent
        {
            get => (View)GetValue(EffectContentProperty);
            set => SetValue(EffectContentProperty, value);
        }

        public BaseEffectView()
        {
            InitializeComponent();
            RootGrid.BindingContext = this;
        }

        private static void OnEffectContentChanged(BindableObject bindable, object oldValue, object newValue)
        {
            if (bindable is BaseEffectView baseEffectView && newValue is View view)
            {
                baseEffectView.ContentBorder.Content = view;
            }
        }
    }
}

