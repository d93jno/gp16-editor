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
            ProgressLabel.Text = $"Patches parsed: {value} / {max}";
        }

        public void SetByteProgress(int bytes, int totalBytes)
        {
            BytesProgressBar.Progress = (double)bytes / totalBytes;
            BytesLabel.Text = $"Data received: {bytes} / {totalBytes} bytes";
        }
    }
}
