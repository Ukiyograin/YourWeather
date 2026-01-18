using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Threading.Tasks;
using System.Windows.Input;
using WeatherApp.Models;
using WeatherApp.Services;

namespace WeatherApp.ViewModels
{
    public class MainViewModel : INotifyPropertyChanged
    {
        private readonly IWeatherService _weatherService;
        private WeatherData _currentWeather = new();
        private string _searchQuery = string.Empty;
        private bool _isLoading;
        private string _statusMessage = string.Empty;
        private Location[] _locationSuggestions = Array.Empty<Location>();
        private Location? _selectedLocation;

        public WeatherData CurrentWeather
        {
            get => _currentWeather;
            set { _currentWeather = value; OnPropertyChanged(); }
        }

        public string SearchQuery
        {
            get => _searchQuery;
            set { _searchQuery = value; OnPropertyChanged(); }
        }

        public bool IsLoading
        {
            get => _isLoading;
            set { _isLoading = value; OnPropertyChanged(); }
        }

        public string StatusMessage
        {
            get => _statusMessage;
            set { _statusMessage = value; OnPropertyChanged(); }
        }

        public Location[] LocationSuggestions
        {
            get => _locationSuggestions;
            set { _locationSuggestions = value; OnPropertyChanged(); }
        }

        public Location? SelectedLocation
        {
            get => _selectedLocation;
            set
            {
                _selectedLocation = value;
                OnPropertyChanged();
                if (value != null)
                {
                    _ = LoadWeatherForLocation(value);
                }
            }
        }

        public ObservableCollection<HourlyForecast> HourlyForecasts { get; } = new();
        public ObservableCollection<DailyForecast> DailyForecasts { get; } = new();

        public ICommand SearchCommand { get; }
        public ICommand RefreshCommand { get; }
        public ICommand UseCurrentLocationCommand { get; }

        public event PropertyChangedEventHandler? PropertyChanged;

        public MainViewModel(IWeatherService weatherService)
        {
            _weatherService = weatherService;

            SearchCommand = new RelayCommand(async () => await SearchWeatherAsync());
            RefreshCommand = new RelayCommand(async () => await RefreshWeatherAsync());
            UseCurrentLocationCommand = new RelayCommand(async () => await UseCurrentLocationAsync());

            // 初始化默认城市
            _ = LoadDefaultWeatherAsync();
        }

        private async Task LoadDefaultWeatherAsync()
        {
            try
            {
                await LoadWeatherForCity("北京");
            }
            catch (Exception ex)
            {
                StatusMessage = $"加载默认天气失败: {ex.Message}";
            }
        }

        private async Task SearchWeatherAsync()
        {
            if (string.IsNullOrWhiteSpace(SearchQuery))
                return;

            await LoadWeatherForCity(SearchQuery);
        }

        private async Task RefreshWeatherAsync()
        {
            if (!string.IsNullOrEmpty(CurrentWeather.City))
            {
                await LoadWeatherForCity(CurrentWeather.City);
            }
        }

        private async Task UseCurrentLocationAsync()
        {
            try
            {
                IsLoading = true;
                StatusMessage = "获取当前位置...";

                // 这里应该使用系统的地理位置API
                // 简化：使用默认坐标（北京）
                var weather = await _weatherService.GetWeatherByCoordinatesAsync(39.9042, 116.4074);
                UpdateWeatherData(weather);
                
                StatusMessage = $"已更新 {weather.City} 的天气";
            }
            catch (Exception ex)
            {
                StatusMessage = $"获取当前位置失败: {ex.Message}";
            }
            finally
            {
                IsLoading = false;
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
                
                StatusMessage = $"已更新 {weather.City} 的天气";
            }
            catch (Exception ex)
            {
                StatusMessage = $"获取天气失败: {ex.Message}";
            }
            finally
            {
                IsLoading = false;
            }
        }

        private async Task LoadWeatherForLocation(Location location)
        {
            try
            {
                IsLoading = true;
                StatusMessage = $"正在获取 {location.Name} 的天气...";

                var weather = await _weatherService.GetWeatherByCoordinatesAsync(
                    location.Latitude, location.Longitude);
                
                weather.City = location.Name;
                weather.Country = location.Country;
                UpdateWeatherData(weather);
                
                StatusMessage = $"已更新 {weather.City} 的天气";
            }
            catch (Exception ex)
            {
                StatusMessage = $"获取天气失败: {ex.Message}";
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

        public RelayCommand(Action execute, Func<bool>? canExecute = null)
        {
            _execute = execute ?? throw new ArgumentNullException(nameof(execute));
            _canExecute = canExecute;
        }

        public bool CanExecute(object? parameter) => _canExecute?.Invoke() ?? true;

        public void Execute(object? parameter) => _execute();

        public void RaiseCanExecuteChanged() => CanExecuteChanged?.Invoke(this, EventArgs.Empty);
    }
}