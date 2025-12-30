using Microsoft.Maui.Controls;
using SkiaSharp;
using SkiaSharp.Views.Maui;
using SkiaSharp.Views.Maui.Controls;

namespace GP16Editor.Controls;

public class CircularSlider : ContentView
{
    private readonly SKCanvasView _canvasView;
    private float _angle;
    private bool _isDragging;

    public static readonly BindableProperty MinimumProperty =
        BindableProperty.Create(nameof(Minimum), typeof(double), typeof(CircularSlider), 0.0, propertyChanged: OnValueChanged);

    public static readonly BindableProperty MaximumProperty =
        BindableProperty.Create(nameof(Maximum), typeof(double), typeof(CircularSlider), 100.0, propertyChanged: OnValueChanged);

    public static readonly BindableProperty ValueProperty =
        BindableProperty.Create(nameof(Value), typeof(double), typeof(CircularSlider), 0.0, BindingMode.TwoWay, propertyChanged: OnValueChanged);

    public static readonly BindableProperty TextColorProperty =
        BindableProperty.Create(nameof(TextColor), typeof(Color), typeof(CircularSlider), Colors.Black);

    public double Minimum
    {
        get => (double)GetValue(MinimumProperty);
        set => SetValue(MinimumProperty, value);
    }

    public double Maximum
    {
        get => (double)GetValue(MaximumProperty);
        set => SetValue(MaximumProperty, value);
    }

    public double Value
    {
        get => (double)GetValue(ValueProperty);
        set => SetValue(ValueProperty, Math.Clamp(value, Minimum, Maximum));
    }

    public Color TextColor
    {
        get => (Color)GetValue(TextColorProperty);
        set => SetValue(TextColorProperty, value);
    }

    public CircularSlider()
    {
        _canvasView = new SKCanvasView();
        _canvasView.PaintSurface += OnPaintSurface;

        var panGesture = new PanGestureRecognizer();
        panGesture.PanUpdated += OnPanUpdated;
        _canvasView.GestureRecognizers.Add(panGesture);

        var valueLabel = new Label
        {
            HorizontalTextAlignment = TextAlignment.Center,
            FontSize = 16,
            TextColor = TextColor
        };
        valueLabel.SetBinding(Label.TextProperty, new Binding(nameof(Value), source: this, stringFormat: "{0:F1}"));

        Content = new VerticalStackLayout
        {
            Spacing = 10,
            Children =
            {
                _canvasView,
                valueLabel
            }
        };

        UpdateAngleFromValue();
    }

    private static void OnValueChanged(BindableObject bindable, object oldValue, object newValue)
    {
        var slider = (CircularSlider)bindable;
        slider.UpdateAngleFromValue();
        slider._canvasView.InvalidateSurface();
    }

    private void UpdateAngleFromValue()
    {
        if (Maximum > Minimum)
        {
            var normalizedValue = (Value - Minimum) / (Maximum - Minimum);
            _angle = (float)(normalizedValue * 270 - 135); // -135 to 135 degrees (270 degree range)
        }
    }

    private void UpdateValueFromAngle()
    {
        var normalizedAngle = (_angle + 135) / 270; // Convert from -135..135 to 0..1
        Value = Minimum + normalizedAngle * (Maximum - Minimum);
    }

    private void OnPanUpdated(object? sender, PanUpdatedEventArgs e)
    {
        switch (e.StatusType)
        {
            case GestureStatus.Started:
                _isDragging = true;
                break;
            case GestureStatus.Running:
                if (_isDragging)
                {
                    var centerX = _canvasView.Width / 2;
                    var centerY = _canvasView.Height / 2;
                    var touchX = e.TotalX;
                    var touchY = e.TotalY;

                    var deltaX = touchX - centerX;
                    var deltaY = touchY - centerY;
                    var newAngle = (float)(Math.Atan2(deltaY, deltaX) * 180 / Math.PI);

                    // Constrain to 270 degree range (-135 to 135)
                    if (newAngle < -135) newAngle = -135;
                    if (newAngle > 135) newAngle = 135;

                    _angle = newAngle;
                    UpdateValueFromAngle();
                }
                break;
            case GestureStatus.Completed:
            case GestureStatus.Canceled:
                _isDragging = false;
                break;
        }
    }

    private void OnPaintSurface(object? sender, SKPaintSurfaceEventArgs e)
    {
        var canvas = e.Surface.Canvas;
        var width = e.Info.Width;
        var height = e.Info.Height;

        canvas.Clear(SKColors.Transparent);

        var centerX = width / 2f;
        var centerY = height / 2f;
        var radius = Math.Min(width, height) / 2f - 20;

        // Draw outer circle
        using var outerCirclePaint = new SKPaint
        {
            Style = SKPaintStyle.Stroke,
            Color = SKColors.LightGray,
            StrokeWidth = 2
        };
        canvas.DrawCircle(centerX, centerY, radius, outerCirclePaint);

        // Draw arc background
        using var arcBackgroundPaint = new SKPaint
        {
            Style = SKPaintStyle.Stroke,
            Color = SKColors.LightGray.WithAlpha(100),
            StrokeWidth = 8,
            StrokeCap = SKStrokeCap.Round
        };
        var arcRect = new SKRect(centerX - radius + 10, centerY - radius + 10, centerX + radius - 10, centerY + radius - 10);
        canvas.DrawArc(arcRect, -135, 270, false, arcBackgroundPaint);

        // Draw value arc
        var sweepAngle = (_angle + 135) / 270 * 270; // Convert angle to sweep
        using var arcPaint = new SKPaint
        {
            Style = SKPaintStyle.Stroke,
            Color = SKColors.CornflowerBlue,
            StrokeWidth = 8,
            StrokeCap = SKStrokeCap.Round
        };
        canvas.DrawArc(arcRect, -135, sweepAngle, false, arcPaint);

        // Draw indicator line
        var indicatorAngle = _angle * (float)(Math.PI / 180);
        var indicatorLength = radius - 15;
        var indicatorX = centerX + (float)Math.Cos(indicatorAngle) * indicatorLength;
        var indicatorY = centerY + (float)Math.Sin(indicatorAngle) * indicatorLength;

        using var indicatorPaint = new SKPaint
        {
            Style = SKPaintStyle.Stroke,
            Color = SKColors.CornflowerBlue,
            StrokeWidth = 3,
            StrokeCap = SKStrokeCap.Round
        };
        canvas.DrawLine(centerX, centerY, indicatorX, indicatorY, indicatorPaint);

        // Draw center dot
        using var centerDotPaint = new SKPaint
        {
            Style = SKPaintStyle.Fill,
            Color = SKColors.CornflowerBlue
        };
        canvas.DrawCircle(centerX, centerY, 5, centerDotPaint);
    }
}