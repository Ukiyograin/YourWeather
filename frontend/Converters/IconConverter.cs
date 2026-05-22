using System;
using System.Globalization;
using System.Windows;
using System.Windows.Data;
using System.Windows.Media;

namespace WeatherApp.Converters
{
    public class IconNameToEmojiConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return (value as string) switch
            {
                "sunny" => "☀",
                "clear-night" => "☾",
                "partly-cloudy-day" => "⛅",
                "partly-cloudy-night" => "☁",
                "cloudy" => "☁",
                "fog" => "≋",
                "drizzle" => "🌦",
                "rain" => "☔",
                "snow" => "❄",
                "thunderstorm" => "⚡",
                _ => "?"
            };
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture) => throw new NotSupportedException();
    }

    public class IconNameToPathDataConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return Geometry.Parse("M12,2A10,10 0 1,1 11.99,2Z");
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture) => throw new NotSupportedException();
    }

    public class BooleanToVisibilityConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return value is bool b && b ? Visibility.Visible : Visibility.Collapsed;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return value is Visibility visibility && visibility == Visibility.Visible;
        }
    }
}
