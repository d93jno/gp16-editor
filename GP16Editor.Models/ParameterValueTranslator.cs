using System.Globalization;

namespace GP16Editor.Models
{
    public static class ParameterValueTranslator
    {
        public static double TranslateParametricEqHiFreq(int rawValue) => TranslateLinear(rawValue, 0, 100, 2000d, 8000d);

        public static double TranslateParametricEqHighMidFreq(int rawValue) => TranslateLinear(rawValue, 0, 100, 500d, 4000d);

        public static double TranslateParametricEqLowMidFreq(int rawValue) => TranslateLogarithmic(rawValue, 0, 100, 125d, 1000d);

        public static double TranslateParametricEqLowFreq(int rawValue) => TranslateLogarithmic(rawValue, 0, 100, 60d, 250d);

        public static double TranslateParametricEqQ(int rawValue) => TranslateLinear(rawValue, 0, 40, 1d, 5d);

        public static double TranslateParametricEqLevel(int rawValue) => TranslateLinear(rawValue, 0, 48, -12d, 12d);

        public static double TranslateLinear(double rawValue, double rawMin, double rawMax, double displayMin, double displayMax)
        {
            if (rawMax <= rawMin)
            {
                return displayMin;
            }

            var clampedValue = Math.Clamp(rawValue, rawMin, rawMax);
            var normalizedValue = (clampedValue - rawMin) / (rawMax - rawMin);
            return displayMin + ((displayMax - displayMin) * normalizedValue);
        }

        public static double TranslateLogarithmic(double rawValue, double rawMin, double rawMax, double displayMin, double displayMax)
        {
            if (rawMax <= rawMin || displayMin <= 0d || displayMax <= 0d)
            {
                return displayMin;
            }

            var clampedValue = Math.Clamp(rawValue, rawMin, rawMax);
            var normalizedValue = (clampedValue - rawMin) / (rawMax - rawMin);
            var ratio = displayMax / displayMin;
            return displayMin * Math.Pow(ratio, normalizedValue);
        }

        public static string FormatFrequency(double hz)
        {
            if (hz >= 1000d)
            {
                return string.Create(CultureInfo.InvariantCulture, $"{hz / 1000d:0.00}kHz");
            }

            return string.Create(CultureInfo.InvariantCulture, $"{Math.Floor(hz):0}Hz");
        }

        public static string FormatDecibel(double db)
        {
            return string.Create(CultureInfo.InvariantCulture, $"{db:+0.0;-0.0;0.0}dB");
        }

        public static string FormatQ(double q)
        {
            return string.Create(CultureInfo.InvariantCulture, $"{q:0.0}");
        }
    }
}
