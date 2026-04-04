using System.Globalization;
using GP16Editor.Models;
using Microsoft.Maui.Controls;
using SkiaSharp;
using SkiaSharp.Views.Maui;
using SkiaSharp.Views.Maui.Controls;

namespace GP16Editor.Controls;

public class CircularSlider : ContentView
{
    private readonly SKCanvasView _canvasView;
    private readonly Label _valueLabel;
    private float _angle;
    private bool _isDragging;
    private float _dragStartAngle;
    private string _currentDisplayText = string.Empty;

    public static readonly BindableProperty MinimumProperty =
        BindableProperty.Create(nameof(Minimum), typeof(double), typeof(CircularSlider), 0.0, propertyChanged: OnValueChanged);

    public static readonly BindableProperty MaximumProperty =
        BindableProperty.Create(nameof(Maximum), typeof(double), typeof(CircularSlider), 100.0, propertyChanged: OnValueChanged);

    public static readonly BindableProperty ValueProperty =
        BindableProperty.Create(nameof(Value), typeof(double), typeof(CircularSlider), 0.0, BindingMode.TwoWay, propertyChanged: OnValueChanged);

    public static readonly BindableProperty LabelTextProperty =
        BindableProperty.Create(nameof(LabelText), typeof(string), typeof(CircularSlider), string.Empty);

    public static readonly BindableProperty DisplayMinimumProperty =
        BindableProperty.Create(nameof(DisplayMinimum), typeof(double), typeof(CircularSlider), double.NaN, propertyChanged: OnDisplaySettingsChanged);

    public static readonly BindableProperty DisplayMaximumProperty =
        BindableProperty.Create(nameof(DisplayMaximum), typeof(double), typeof(CircularSlider), double.NaN, propertyChanged: OnDisplaySettingsChanged);

    public static readonly BindableProperty DisplayAsFrequencyProperty =
        BindableProperty.Create(nameof(DisplayAsFrequency), typeof(bool), typeof(CircularSlider), false, propertyChanged: OnDisplaySettingsChanged);

    public static readonly BindableProperty DisplayUnitProperty =
        BindableProperty.Create(nameof(DisplayUnit), typeof(string), typeof(CircularSlider), string.Empty, propertyChanged: OnDisplaySettingsChanged);

    public static readonly BindableProperty DisplayFormatProperty =
        BindableProperty.Create(nameof(DisplayFormat), typeof(string), typeof(CircularSlider), string.Empty, propertyChanged: OnDisplaySettingsChanged);

    public static readonly BindableProperty DisplaySignedProperty =
        BindableProperty.Create(nameof(DisplaySigned), typeof(bool), typeof(CircularSlider), false, propertyChanged: OnDisplaySettingsChanged);

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

    public string LabelText
    {
        get => (string)GetValue(LabelTextProperty);
        set => SetValue(LabelTextProperty, value);
    }

    public double DisplayMinimum
    {
        get => (double)GetValue(DisplayMinimumProperty);
        set => SetValue(DisplayMinimumProperty, value);
    }

    public double DisplayMaximum
    {
        get => (double)GetValue(DisplayMaximumProperty);
        set => SetValue(DisplayMaximumProperty, value);
    }

    public bool DisplayAsFrequency
    {
        get => (bool)GetValue(DisplayAsFrequencyProperty);
        set => SetValue(DisplayAsFrequencyProperty, value);
    }

    public string DisplayUnit
    {
        get => (string)GetValue(DisplayUnitProperty);
        set => SetValue(DisplayUnitProperty, value);
    }

    public string DisplayFormat
    {
        get => (string)GetValue(DisplayFormatProperty);
        set => SetValue(DisplayFormatProperty, value);
    }

    public bool DisplaySigned
    {
        get => (bool)GetValue(DisplaySignedProperty);
        set => SetValue(DisplaySignedProperty, value);
    }

    public CircularSlider()
    {
        _canvasView = new SKCanvasView
        {
            WidthRequest = 100,
            HeightRequest = 100
        };
        _canvasView.PaintSurface += OnPaintSurface;

        var nameLabel = new Label
        {
            HorizontalTextAlignment = TextAlignment.Center,
            FontSize = 18,
        };
        nameLabel.SetBinding(Label.TextProperty, new Binding(nameof(LabelText), source: this));
        nameLabel.SetDynamicResource(Label.TextColorProperty, "TextColor");

        _valueLabel = new Label
        {
            HorizontalTextAlignment = TextAlignment.Center,
            FontSize = 16,
        };
        _valueLabel.SetDynamicResource(Label.TextColorProperty, "TextColor");

        Content = new VerticalStackLayout
        {
            Spacing = 10,
            Children =
            {
                nameLabel,
                _canvasView,
                _valueLabel
            }
        };

        UpdateAngleFromValue();
        UpdateValueLabel();
    }

    protected override void OnParentSet()
    {
        base.OnParentSet();
        if (Parent != null)
        {
            var panGesture = new PanGestureRecognizer();
            panGesture.PanUpdated += OnPanUpdated;
            _canvasView.GestureRecognizers.Add(panGesture);
        }
    }

    private static void OnValueChanged(BindableObject bindable, object oldValue, object newValue)
    {
        var slider = (CircularSlider)bindable;
        slider.UpdateAngleFromValue();
        slider.UpdateValueLabel();
        slider._canvasView.InvalidateSurface();
    }

    private static void OnDisplaySettingsChanged(BindableObject bindable, object oldValue, object newValue)
    {
        var slider = (CircularSlider)bindable;
        slider.UpdateValueLabel();
        slider._canvasView.InvalidateSurface();
    }

    private void UpdateAngleFromValue()
    {
        if (Maximum > Minimum)
        {
            var clampedValue = Math.Clamp(Value, Minimum, Maximum);
            var normalizedValue = (clampedValue - Minimum) / (Maximum - Minimum);
            _angle = (float)(normalizedValue * 270 - 135);
        }
    }

    private void UpdateValueFromAngle()
    {
        var normalizedAngle = (_angle + 135) / 270;
        var calculatedValue = Minimum + normalizedAngle * (Maximum - Minimum);
        Value = UsesIntegerStep()
            ? Math.Round(calculatedValue)
            : Math.Round(calculatedValue, 2);
    }

    private void UpdateValueLabel()
    {
        var displayText = FormatDisplayValue(Value);
        if (_currentDisplayText != displayText)
        {
            _currentDisplayText = displayText;
            _valueLabel.Text = displayText;
        }
    }

    private bool UsesIntegerStep()
    {
        return IsWholeNumber(Minimum) && IsWholeNumber(Maximum);
    }

    private static bool IsWholeNumber(double value)
    {
        return Math.Abs(value - Math.Round(value)) < 0.000001d;
    }

    private string FormatDisplayValue(double value)
    {
        var translatedDisplayValue = TranslateDisplayValue(value);
        if (DisplayAsFrequency)
        {
            return ParameterValueTranslator.FormatFrequency(translatedDisplayValue);
        }

        if (!string.IsNullOrWhiteSpace(DisplayFormat))
        {
            var formattedValue = translatedDisplayValue.ToString(DisplayFormat, CultureInfo.InvariantCulture);
            formattedValue = FormatSignedNumber(formattedValue, translatedDisplayValue);
            return string.IsNullOrEmpty(DisplayUnit) ? formattedValue : $"{formattedValue}{DisplayUnit}";
        }

        if (HasDisplayRange())
        {
            var roundedDisplayValue = Math.Round(translatedDisplayValue, 2);
            if (IsWholeNumber(roundedDisplayValue))
            {
                var integerText = FormatSignedNumber(((int)Math.Round(roundedDisplayValue)).ToString(CultureInfo.InvariantCulture), roundedDisplayValue);
                return string.IsNullOrEmpty(DisplayUnit) ? integerText : $"{integerText}{DisplayUnit}";
            }

            var decimalText = FormatSignedNumber(roundedDisplayValue.ToString("0.##", CultureInfo.InvariantCulture), roundedDisplayValue);
            return string.IsNullOrEmpty(DisplayUnit) ? decimalText : $"{decimalText}{DisplayUnit}";
        }

        if (UsesIntegerStep())
        {
            var integerText = FormatSignedNumber(((int)Math.Round(value)).ToString(CultureInfo.InvariantCulture), value);
            return string.IsNullOrEmpty(DisplayUnit) ? integerText : $"{integerText}{DisplayUnit}";
        }

        var roundedValue = Math.Round(value, 2);
        var rawDecimalText = FormatSignedNumber(roundedValue.ToString("0.##", CultureInfo.InvariantCulture), roundedValue);
        return string.IsNullOrEmpty(DisplayUnit) ? rawDecimalText : $"{rawDecimalText}{DisplayUnit}";
    }

    private string FormatSignedNumber(string text, double value)
    {
        if (DisplaySigned && value > 0)
        {
            return $"+{text}";
        }

        return text;
    }

    private bool HasDisplayRange()
    {
        return !double.IsNaN(DisplayMinimum) && !double.IsNaN(DisplayMaximum);
    }

    private double TranslateDisplayValue(double rawValue)
    {
        if (!HasDisplayRange())
        {
            return rawValue;
        }

        if (Maximum <= Minimum)
        {
            return DisplayMinimum;
        }

        return ParameterValueTranslator.TranslateLinear(rawValue, Minimum, Maximum, DisplayMinimum, DisplayMaximum);
    }

    private void OnPanUpdated(object? sender, PanUpdatedEventArgs e)
    {
        switch (e.StatusType)
        {
            case GestureStatus.Started:
                _isDragging = true;
                _dragStartAngle = _angle;
                break;
            case GestureStatus.Running:
                if (_isDragging)
                {
                    var newAngle = _dragStartAngle + (float)e.TotalX;
                    _angle = Math.Clamp(newAngle, -135f, 135f);
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
        var clampedValue = Maximum > Minimum ? Math.Clamp(Value, Minimum, Maximum) : Minimum;
        var normalizedValue = Maximum > Minimum ? (float)((clampedValue - Minimum) / (Maximum - Minimum)) : 0f;
        var sweepAngle = normalizedValue * 270f;
        var indicatorAngleDegrees = -135f + sweepAngle;
        var displayText = FormatDisplayValue(clampedValue);
        if (_currentDisplayText != displayText)
        {
            _currentDisplayText = displayText;
            _valueLabel.Text = displayText;
        }

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
        using var arcPaint = new SKPaint
        {
            Style = SKPaintStyle.Stroke,
            Color = SKColors.CornflowerBlue,
            StrokeWidth = 8,
            StrokeCap = SKStrokeCap.Round
        };
        canvas.DrawArc(arcRect, -135, sweepAngle, false, arcPaint);

        // Draw indicator line
        var indicatorAngle = indicatorAngleDegrees * (float)(Math.PI / 180);
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
