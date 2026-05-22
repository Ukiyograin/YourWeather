using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO;
using System.Runtime.CompilerServices;
using System.Text.Json;
using System.Threading.Tasks;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Threading;
using WeatherApp.Models;
using WeatherApp.Services;

namespace WeatherApp.ViewModels
{
    public class MainViewModel : INotifyPropertyChanged
    {
        private readonly IWeatherService _weatherService;
        private readonly DispatcherTimer _refreshTimer;
        private WeatherData _currentWeather = new();
        private string _searchQuery = "北京";
        private bool _isLoading;
        private string _statusMessage = "输入城市并搜索天气";
        private bool _useFahrenheit;
        private bool _darkTheme;

        public WeatherData CurrentWeather { get => _currentWeather; set { _currentWeather = value; OnPropertyChanged(); RefreshDisplayProperties(); } }
        public string SearchQuery { get => _searchQuery; set { _searchQuery = value; OnPropertyChanged(); } }
        public bool IsLoading { get => _isLoading; set { _isLoading = value; OnPropertyChanged(); } }
        public string StatusMessage { get => _statusMessage; set { _statusMessage = value; OnPropertyChanged(); } }

        public ObservableCollection<HourlyForecast> HourlyForecasts { get; } = new();
        public ObservableCollection<DailyForecast> DailyForecasts { get; } = new();
        public ObservableCollection<string> RecentCities { get; } = new();

        public string DisplayTemperature => $"{ConvertTemperature(CurrentWeather.Temperature):F0}°{(_useFahrenheit ? "F" : "C")}";
        public string DisplayFeelsLike => $"体感 {ConvertTemperature(CurrentWeather.FeelsLike):F0}°{(_useFahrenheit ? "F" : "C")}";
        public string DisplayWindSpeed => _useFahrenheit ? $"{CurrentWeather.WindSpeed / 1.609:F1} mph" : $"{CurrentWeather.WindSpeed:F1} km/h";

        public Brush WindowBackground => new SolidColorBrush((Color)ColorConverter.ConvertFromString(_darkTheme ? "#101827" : "#F5F7FA"));
        public Brush HeaderBackground => new SolidColorBrush((Color)ColorConverter.ConvertFromString(_darkTheme ? "#1E3A8A" : "#2196F3"));
        public Brush CardBackground => new SolidColorBrush((Color)ColorConverter.ConvertFromString(_darkTheme ? "#1F2937" : "#FFFFFF"));
        public Brush SubtleBackground => new SolidColorBrush((Color)ColorConverter.ConvertFromString(_darkTheme ? "#111827" : "#EEF6FF"));
        public Brush PrimaryText => new SolidColorBrush((Color)ColorConverter.ConvertFromString(_darkTheme ? "#F9FAFB" : "#1F2937"));
        public Brush SecondaryText => new SolidColorBrush((Color)ColorConverter.ConvertFromString(_darkTheme ? "#CBD5E1" : "#64748B"));
        public Brush AccentBrush => new SolidColorBrush((Color)ColorConverter.ConvertFromString(_darkTheme ? "#93C5FD" : "#0EA5E9"));

        public ICommand SearchCommand { get; }
        public ICommand RefreshCommand { get; }
        public ICommand ToggleUnitsCommand { get; }
        public ICommand ToggleThemeCommand { get; }
        public ICommand LoadRecentCityCommand { get; }

        public event PropertyChangedEventHandler? PropertyChanged;

        public MainViewModel() : this(new WeatherService())
        {
        }

        public MainViewModel(IWeatherService weatherService)
        {
            _weatherService = weatherService;
            SearchCommand = new RelayCommand(async () => await SearchWeatherAsync());
            RefreshCommand = new RelayCommand(async () => await RefreshWeatherAsync());
            ToggleUnitsCommand = new RelayCommand(ToggleUnits);
            ToggleThemeCommand = new RelayCommand(ToggleTheme);
            LoadRecentCityCommand = new RelayCommand<string>(async city => await LoadWeatherForCity(city));

            LoadRecentCities();
            _refreshTimer = new DispatcherTimer { Interval = TimeSpan.FromMinutes(10) };
            _refreshTimer.Tick += async (_, _) => await RefreshWeatherAsync();
            _refreshTimer.Start();

            _ = LoadWeatherForCity(SearchQuery);
        }

        private async Task SearchWeatherAsync()
        {
            if (!string.IsNullOrWhiteSpace(SearchQuery))
            {
                await LoadWeatherForCity(SearchQuery.Trim());
            }
        }

        private async Task RefreshWeatherAsync()
        {
            if (!string.IsNullOrWhiteSpace(CurrentWeather.City))
            {
                await LoadWeatherForCity(CurrentWeather.City);
            }
        }

        private async Task LoadWeatherForCity(string city)
        {
            try
            {
                IsLoading = true;
                StatusMessage = $"正在获取 {city} 的天气...";
                var weather = await _weatherService.GetWeatherForecastAsync(city, 7);
                UpdateWeatherData(weather);
                SearchQuery = weather.City;
                AddRecentCity(weather.City);
                StatusMessage = $"已更新：{DateTime.Now:HH:mm:ss}";
            }
            catch (Exception ex)
            {
                StatusMessage = ex.Message;
            }
            finally
            {
                IsLoading = false;
            }
        }

        private void UpdateWeatherData(WeatherData weather)
        {
            CurrentWeather = weather;
            HourlyForecasts.Clear();
            foreach (var hourly in weather.HourlyForecast)
            {
                HourlyForecasts.Add(hourly);
            }

            DailyForecasts.Clear();
            foreach (var daily in weather.DailyForecast)
            {
                DailyForecasts.Add(daily);
            }
        }

        private void ToggleUnits()
        {
            _useFahrenheit = !_useFahrenheit;
            RefreshDisplayProperties();
        }

        private void ToggleTheme()
        {
            _darkTheme = !_darkTheme;
            OnPropertyChanged(nameof(WindowBackground));
            OnPropertyChanged(nameof(HeaderBackground));
            OnPropertyChanged(nameof(CardBackground));
            OnPropertyChanged(nameof(SubtleBackground));
            OnPropertyChanged(nameof(PrimaryText));
            OnPropertyChanged(nameof(SecondaryText));
            OnPropertyChanged(nameof(AccentBrush));
        }

        private double ConvertTemperature(double celsius) => _useFahrenheit ? celsius * 9 / 5 + 32 : celsius;

        private void RefreshDisplayProperties()
        {
            OnPropertyChanged(nameof(DisplayTemperature));
            OnPropertyChanged(nameof(DisplayFeelsLike));
            OnPropertyChanged(nameof(DisplayWindSpeed));
        }

        private void LoadRecentCities()
        {
            try
            {
                var path = SettingsPath;
                if (!File.Exists(path)) return;
                var cities = JsonSerializer.Deserialize<string[]>(File.ReadAllText(path)) ?? Array.Empty<string>();
                foreach (var city in cities)
                {
                    if (!string.IsNullOrWhiteSpace(city)) RecentCities.Add(city);
                }
            }
            catch
            {
            }
        }

        private void AddRecentCity(string city)
        {
            if (string.IsNullOrWhiteSpace(city)) return;
            RecentCities.Remove(city);
            RecentCities.Insert(0, city);
            while (RecentCities.Count > 8) RecentCities.RemoveAt(RecentCities.Count - 1);
            SaveRecentCities();
        }

        private void SaveRecentCities()
        {
            try
            {
                Directory.CreateDirectory(Path.GetDirectoryName(SettingsPath)!);
                File.WriteAllText(SettingsPath, JsonSerializer.Serialize(RecentCities));
            }
            catch
            {
            }
        }

        private static string SettingsPath => Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "YourWeather", "settings.json");

        protected virtual void OnPropertyChanged([CallerMemberName] string? propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }

    public class RelayCommand : ICommand
    {
        private readonly Action _execute;
        private readonly Func<bool>? _canExecute;
        public event EventHandler? CanExecuteChanged;
        public RelayCommand(Action execute, Func<bool>? canExecute = null) { _execute = execute; _canExecute = canExecute; }
        public bool CanExecute(object? parameter) => _canExecute?.Invoke() ?? true;
        public void Execute(object? parameter) => _execute();
    }

    public class RelayCommand<T> : ICommand
    {
        private readonly Func<T, Task> _execute;
        public event EventHandler? CanExecuteChanged;
        public RelayCommand(Func<T, Task> execute) { _execute = execute; }
        public bool CanExecute(object? parameter) => parameter is T;
        public async void Execute(object? parameter)
        {
            if (parameter is T value) await _execute(value);
        }
    }
}
