using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace WeatherApp.Models
{
    public class WeatherData : INotifyPropertyChanged
    {
        private double _temperature;
        private double _feelsLike;
        private int _humidity;
        private double _windSpeed;
        private int _windDirection;
        private double _pressure;
        private double _precipitation;
        private int _uvIndex;
        private string _condition = "等待加载";
        private string _description = string.Empty;
        private string _iconName = "unknown";
        private string _city = "北京";
        private string _country = "中国";
        private DateTime _timestamp = DateTime.Now;

        public double Temperature { get => _temperature; set { _temperature = value; OnPropertyChanged(); } }
        public double FeelsLike { get => _feelsLike; set { _feelsLike = value; OnPropertyChanged(); } }
        public int Humidity { get => _humidity; set { _humidity = value; OnPropertyChanged(); } }
        public double WindSpeed { get => _windSpeed; set { _windSpeed = value; OnPropertyChanged(); } }
        public int WindDirection { get => _windDirection; set { _windDirection = value; OnPropertyChanged(); } }
        public double Pressure { get => _pressure; set { _pressure = value; OnPropertyChanged(); } }
        public double Precipitation { get => _precipitation; set { _precipitation = value; OnPropertyChanged(); } }
        public int UvIndex { get => _uvIndex; set { _uvIndex = value; OnPropertyChanged(); } }
        public string Condition { get => _condition; set { _condition = value; OnPropertyChanged(); } }
        public string Description { get => _description; set { _description = value; OnPropertyChanged(); } }
        public string IconName { get => _iconName; set { _iconName = value; OnPropertyChanged(); } }
        public string City { get => _city; set { _city = value; OnPropertyChanged(); OnPropertyChanged(nameof(LocationText)); } }
        public string Country { get => _country; set { _country = value; OnPropertyChanged(); OnPropertyChanged(nameof(LocationText)); } }
        public DateTime Timestamp { get => _timestamp; set { _timestamp = value; OnPropertyChanged(); } }
        public string LocationText => string.IsNullOrWhiteSpace(Country) ? City : $"{City}, {Country}";

        public List<HourlyForecast> HourlyForecast { get; set; } = new();
        public List<DailyForecast> DailyForecast { get; set; } = new();

        public event PropertyChangedEventHandler? PropertyChanged;
        protected virtual void OnPropertyChanged([CallerMemberName] string? propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }

    public class HourlyForecast
    {
        public DateTime Time { get; set; }
        public double Temperature { get; set; }
        public double PrecipitationProbability { get; set; }
        public string IconName { get; set; } = "unknown";
    }

    public class DailyForecast
    {
        public DateTime Date { get; set; }
        public double TempMax { get; set; }
        public double TempMin { get; set; }
        public double Precipitation { get; set; }
        public string IconName { get; set; } = "unknown";
        public string Condition { get; set; } = string.Empty;
        public DateTime Sunrise { get; set; }
        public DateTime Sunset { get; set; }
        public string TempRange => $"{TempMin:F0}° / {TempMax:F0}°";
    }

    public class Location
    {
        public string Name { get; set; } = string.Empty;
        public string Country { get; set; } = string.Empty;
        public double Latitude { get; set; }
        public double Longitude { get; set; }
    }
}
