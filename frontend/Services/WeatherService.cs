using System;
using System.Net.Http;
using System.Text.Json;
using System.Threading.Tasks;
using WeatherApp.Models;

namespace WeatherApp.Services
{
    public interface IWeatherService
    {
        Task<WeatherData> GetCurrentWeatherAsync(string city, string country = "");
        Task<WeatherData> GetWeatherForecastAsync(string city, int days = 3, string country = "");
        Task<Location[]> SearchLocationsAsync(string query);
        Task<WeatherData> GetWeatherByCoordinatesAsync(double lat, double lon);
    }

    public class WeatherService : IWeatherService
    {
        private readonly HttpClient _httpClient;
        private const string BaseUrl = "https://api.open-meteo.com/v1";

        public WeatherService()
        {
            _httpClient = new HttpClient
            {
                Timeout = TimeSpan.FromSeconds(10)
            };
            _httpClient.DefaultRequestHeaders.UserAgent.ParseAdd("WeatherApp/1.0");
        }

        public async Task<WeatherData> GetCurrentWeatherAsync(string city, string country = "")
        {
            try
            {
                // 先获取坐标
                var location = await GetCoordinatesAsync(city, country);
                if (location == null)
                    throw new Exception("无法找到该城市");

                // 获取天气数据
                string url = $"{BaseUrl}/forecast?" +
                           $"latitude={location.Latitude}&longitude={location.Longitude}&" +
                           "current=temperature_2m,relative_humidity_2m,apparent_temperature," +
                           "wind_speed_10m,wind_direction_10m,pressure_msl,precipitation," +
                           "cloud_cover,weather_code,is_day&" +
                           "timezone=auto&language=zh";

                var response = await _httpClient.GetStringAsync(url);
                var weatherData = ParseWeatherData(response, location);

                return weatherData;
            }
            catch (Exception ex)
            {
                throw new Exception($"获取天气数据失败: {ex.Message}", ex);
            }
        }

        public async Task<WeatherData> GetWeatherForecastAsync(string city, int days = 3, string country = "")
        {
            try
            {
                var location = await GetCoordinatesAsync(city, country);
                if (location == null)
                    throw new Exception("无法找到该城市");

                string url = $"{BaseUrl}/forecast?" +
                           $"latitude={location.Latitude}&longitude={location.Longitude}&" +
                           "current=temperature_2m,relative_humidity_2m,apparent_temperature," +
                           "wind_speed_10m,wind_direction_10m,pressure_msl,precipitation," +
                           "cloud_cover,weather_code,is_day&" +
                           "hourly=temperature_2m,precipitation_probability,weather_code&" +
                           "daily=weather_code,temperature_2m_max,temperature_2m_min," +
                           "precipitation_sum,sunrise,sunset&" +
                           $"forecast_days={days}&timezone=auto&language=zh";

                var response = await _httpClient.GetStringAsync(url);
                var weatherData = ParseWeatherData(response, location);
                
                // 解析预报数据
                var jsonDoc = JsonDocument.Parse(response);
                ParseForecastData(jsonDoc, weatherData);

                return weatherData;
            }
            catch (Exception ex)
            {
                throw new Exception($"获取天气预报失败: {ex.Message}", ex);
            }
        }

        public async Task<Location[]> SearchLocationsAsync(string query)
        {
            try
            {
                string url = $"https://geocoding-api.open-meteo.com/v1/search?" +
                           $"name={Uri.EscapeDataString(query)}&count=10&language=zh&format=json";

                var response = await _httpClient.GetStringAsync(url);
                var jsonDoc = JsonDocument.Parse(response);

                if (jsonDoc.RootElement.TryGetProperty("results", out var results))
                {
                    var locations = new System.Collections.Generic.List<Location>();
                    foreach (var result in results.EnumerateArray())
                    {
                        locations.Add(new Location
                        {
                            Name = result.GetProperty("name").GetString() ?? "",
                            Country = result.GetProperty("country").GetString() ?? "",
                            Latitude = result.GetProperty("latitude").GetDouble(),
                            Longitude = result.GetProperty("longitude").GetDouble()
                        });
                    }
                    return locations.ToArray();
                }

                return Array.Empty<Location>();
            }
            catch (Exception ex)
            {
                throw new Exception($"搜索城市失败: {ex.Message}", ex);
            }
        }

        public async Task<WeatherData> GetWeatherByCoordinatesAsync(double lat, double lon)
        {
            try
            {
                string url = $"{BaseUrl}/forecast?" +
                           $"latitude={lat}&longitude={lon}&" +
                           "current=temperature_2m,relative_humidity_2m,apparent_temperature," +
                           "wind_speed_10m,wind_direction_10m,pressure_msl,precipitation," +
                           "cloud_cover,weather_code,is_day&" +
                           "timezone=auto&language=zh";

                var response = await _httpClient.GetStringAsync(url);
                var weatherData = ParseWeatherData(response, new Location
                {
                    Name = "当前位置",
                    Latitude = lat,
                    Longitude = lon
                });

                return weatherData;
            }
            catch (Exception ex)
            {
                throw new Exception($"获取天气数据失败: {ex.Message}", ex);
            }
        }

        private async Task<Location?> GetCoordinatesAsync(string city, string country)
        {
            try
            {
                string query = city;
                if (!string.IsNullOrEmpty(country))
                    query += $",{country}";

                string url = $"https://geocoding-api.open-meteo.com/v1/search?" +
                           $"name={Uri.EscapeDataString(query)}&count=1&language=zh&format=json";

                var response = await _httpClient.GetStringAsync(url);
                var jsonDoc = JsonDocument.Parse(response);

                if (jsonDoc.RootElement.TryGetProperty("results", out var results) &&
                    results.GetArrayLength() > 0)
                {
                    var result = results[0];
                    return new Location
                    {
                        Name = result.GetProperty("name").GetString() ?? city,
                        Country = result.GetProperty("country").GetString() ?? "",
                        Latitude = result.GetProperty("latitude").GetDouble(),
                        Longitude = result.GetProperty("longitude").GetDouble()
                    };
                }

                return null;
            }
            catch
            {
                return null;
            }
        }

        private WeatherData ParseWeatherData(string json, Location location)
        {
            var jsonDoc = JsonDocument.Parse(json);
            var current = jsonDoc.RootElement.GetProperty("current");

            var weatherCode = current.GetProperty("weather_code").GetInt32();
            var isDay = current.GetProperty("is_day").GetInt32() == 1;

            return new WeatherData
            {
                City = location.Name,
                Country = location.Country,
                Temperature = current.GetProperty("temperature_2m").GetDouble(),
                FeelsLike = current.GetProperty("apparent_temperature").GetDouble(),
                Humidity = current.GetProperty("relative_humidity_2m").GetInt32(),
                WindSpeed = current.GetProperty("wind_speed_10m").GetDouble(),
                Condition = GetConditionFromCode(weatherCode),
                IconName = GetIconNameFromCode(weatherCode, isDay),
                Timestamp = DateTime.UtcNow
            };
        }

        private void ParseForecastData(JsonDocument jsonDoc, WeatherData weatherData)
        {
            // 解析逐小时预报
            if (jsonDoc.RootElement.TryGetProperty("hourly", out var hourly))
            {
                var times = hourly.GetProperty("time");
                var temps = hourly.GetProperty("temperature_2m");
                var precipProbs = hourly.GetProperty("precipitation_probability");
                var weatherCodes = hourly.GetProperty("weather_code");

                int count = Math.Min(Math.Min(times.GetArrayLength(), temps.GetArrayLength()),
                                   Math.Min(precipProbs.GetArrayLength(), weatherCodes.GetArrayLength()));
                count = Math.Min(count, 24); // 限制24小时

                for (int i = 0; i < count; i++)
                {
                    var weatherCode = weatherCodes[i].GetInt32();
                    weatherData.HourlyForecast.Add(new HourlyForecast
                    {
                        Time = DateTime.Parse(times[i].GetString() ?? DateTime.UtcNow.ToString()),
                        Temperature = temps[i].GetDouble(),
                        PrecipitationProbability = precipProbs[i].GetDouble(),
                        IconName = GetIconNameFromCode(weatherCode, true)
                    });
                }
            }

            // 解析每日预报
            if (jsonDoc.RootElement.TryGetProperty("daily", out var daily))
            {
                var dates = daily.GetProperty("time");
                var tempMaxs = daily.GetProperty("temperature_2m_max");
                var tempMins = daily.GetProperty("temperature_2m_min");
                var precipSums = daily.GetProperty("precipitation_sum");
                var weatherCodes = daily.GetProperty("weather_code");
                var sunrises = daily.GetProperty("sunrise");
                var sunsets = daily.GetProperty("sunset");

                int count = Math.Min(Math.Min(dates.GetArrayLength(), tempMaxs.GetArrayLength()),
                                   Math.Min(tempMins.GetArrayLength(), weatherCodes.GetArrayLength()));

                for (int i = 0; i < count; i++)
                {
                    var weatherCode = weatherCodes[i].GetInt32();
                    weatherData.DailyForecast.Add(new DailyForecast
                    {
                        Date = DateTime.Parse(dates[i].GetString() ?? DateTime.UtcNow.ToString()),
                        TempMax = tempMaxs[i].GetDouble(),
                        TempMin = tempMins[i].GetDouble(),
                        Precipitation = precipSums[i].GetDouble(),
                        Condition = GetConditionFromCode(weatherCode),
                        IconName = GetIconNameFromCode(weatherCode, true),
                        Sunrise = DateTime.Parse(sunrises[i].GetString() ?? "06:00"),
                        Sunset = DateTime.Parse(sunsets[i].GetString() ?? "18:00")
                    });
                }
            }
        }

        private string GetConditionFromCode(int code)
        {
            return code switch
            {
                0 => "晴天",
                1 => "大部晴朗",
                2 => "部分多云",
                3 => "阴天",
                45 or 48 => "有雾",
                51 or 56 => "小雨",
                53 or 57 => "中雨",
                55 => "大雨",
                61 or 66 => "小雨",
                63 or 67 => "中雨",
                65 => "大雨",
                71 or 85 => "小雪",
                73 or 86 => "中雪",
                75 => "大雪",
                77 => "雪粒",
                80 => "小阵雨",
                81 => "中阵雨",
                82 => "强阵雨",
                95 => "雷暴",
                96 or 99 => "雹暴",
                _ => "未知"
            };
        }

        private string GetIconNameFromCode(int code, bool isDay)
        {
            if (code == 0) return isDay ? "sunny" : "clear-night";
            if (code >= 1 && code <= 3) return isDay ? "partly-cloudy-day" : "partly-cloudy-night";
            if (code == 45 || code == 48) return "fog";
            if (code >= 51 && code <= 57) return "drizzle";
            if (code >= 61 && code <= 67) return "rain";
            if (code >= 71 && code <= 77) return "snow";
            if (code >= 80 && code <= 82) return "rain";
            if (code >= 85 && code <= 86) return "snow";
            if (code >= 95 && code <= 99) return "thunderstorm";
            return "unknown";
        }
    }
}