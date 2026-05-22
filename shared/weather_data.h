#ifndef WEATHER_DATA_H
#define WEATHER_DATA_H

#include <string>
#include <vector>
#include <cstdint>

struct WeatherData {
    double temperature = 0.0;
    double feels_like = 0.0;
    int humidity = 0;
    double wind_speed = 0.0;
    int wind_direction = 0;
    double pressure = 1013.0;
    double precipitation = 0.0;
    int cloud_cover = 0;
    int uv_index = 0;
    std::string condition;
    std::string description;
    int weather_code = 0;
    std::string icon_name;
    std::string time;
    int64_t timestamp = 0;

    std::string city;
    std::string country;
    double latitude = 0.0;
    double longitude = 0.0;
    std::string timezone;

    struct HourlyData {
        std::string time;
        double temperature = 0.0;
        double precipitation_probability = 0.0;
        int weather_code = 0;
    };

    std::vector<HourlyData> hourly_forecast;

    struct DailyData {
        std::string date;
        double temp_max = 0.0;
        double temp_min = 0.0;
        double precipitation_sum = 0.0;
        int weather_code = 0;
        std::string sunrise;
        std::string sunset;
    };

    std::vector<DailyData> daily_forecast;
};

struct WeatherRequest {
    enum RequestType {
        CURRENT_WEATHER = 0,
        FORECAST = 1,
        SEARCH_CITY = 2,
        GEO_LOCATION = 3
    };

    RequestType type = CURRENT_WEATHER;
    std::string city_name;
    std::string country_code;
    int days = 7;
    double latitude = 0.0;
    double longitude = 0.0;
    std::string language = "zh";
    std::string units = "metric";
};

struct WeatherResponse {
    bool success = false;
    std::string error_message;
    WeatherData current_weather;
    std::vector<WeatherData> forecast;
    std::vector<std::pair<std::string, std::string>> city_suggestions;
};

#endif
