using Microsoft.Maui.Controls;

namespace GP16Editor.Views
{
    public partial class ProgressPopup : CommunityToolkit.Maui.Views.Popup
    {
        public ProgressPopup()
        {
            InitializeComponent();
        }

        public void SetProgress(int value, int max)
        {
            ProgressBar.Progress = (double)value / max;
            ProgressLabel.Text = $"{value} / {max}";
        }
    }
}
