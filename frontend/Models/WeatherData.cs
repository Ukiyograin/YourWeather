using System;
using System.Collections.Generic;
using System.ComponentModel;

namespace WeatherApp.Models
{
    public class WeatherData : INotifyPropertyChanged
    {
        private double _temperature;
        private double _feelsLike;
        private int _humidity;
        private double _windSpeed;
        private string _condition = string.Empty;
        private string _description = string.Empty;
        private string _iconName = string.Empty;
        private string _city = string.Empty;
        private string _country = string.Empty;
        private DateTime _timestamp;

        public double Temperature
        {
            get => _temperature;
            set { _temperature = value; OnPropertyChanged(nameof(Temperature)); }
        }

        public double FeelsLike
        {
            get => _feelsLike;
            set { _feelsLike = value; OnPropertyChanged(nameof(FeelsLike)); }
        }

        public int Humidity
        {
            get => _humidity;
            set { _humidity = value; OnPropertyChanged(nameof(Humidity)); }
        }

        public double WindSpeed
        {
            get => _windSpeed;
            set { _windSpeed = value; OnPropertyChanged(nameof(WindSpeed)); }
        }

        public string Condition
        {
            get => _condition;
            set { _condition = value; OnPropertyChanged(nameof(Condition)); }
        }

        public string Description
        {
            get => _description;
            set { _description = value; OnPropertyChanged(nameof(Description)); }
        }

        public string IconName
        {
            get => _iconName;
            set { _iconName = value; OnPropertyChanged(nameof(IconName)); }
        }

        public string City
        {
            get => _city;
            set { _city = value; OnPropertyChanged(nameof(City)); }
        }

        public string Country
        {
            get => _country;
            set { _country = value; OnPropertyChanged(nameof(Country)); }
        }

        public DateTime Timestamp
        {
            get => _timestamp;
            set { _timestamp = value; OnPropertyChanged(nameof(Timestamp)); }
        }

        public List<HourlyForecast> HourlyForecast { get; set; } = new();
        public List<DailyForecast> DailyForecast { get; set; } = new();

        public event PropertyChangedEventHandler? PropertyChanged;

        protected virtual void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }

    public class HourlyForecast
    {
        public DateTime Time { get; set; }
        public double Temperature { get; set; }
        public double PrecipitationProbability { get; set; }
        public string IconName { get; set; } = string.Empty;
    }

    public class DailyForecast
    {
        public DateTime Date { get; set; }
        public double TempMax { get; set; }
        public double TempMin { get; set; }
        public double Precipitation { get; set; }
        public string IconName { get; set; } = string.Empty;
        public string Condition { get; set; } = string.Empty;
        public DateTime Sunrise { get; set; }
        public DateTime Sunset { get; set; }
    }

    public class Location
    {
        public string Name { get; set; } = string.Empty;
        public string Country { get; set; } = string.Empty;
        public double Latitude { get; set; }
        public double Longitude { get; set; }
    }
}