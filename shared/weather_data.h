#ifndef WEATHER_DATA_H
#define WEATHER_DATA_H

#include <string>
#include <vector>
#include <cstdint>

// 基本天气数据
struct WeatherData {
    double temperature;         // 温度 (°C)
    double feels_like;          // 体感温度
    int humidity;              // 湿度 (%)
    double wind_speed;         // 风速 (km/h)
    int wind_direction;        // 风向 (度)
    double pressure;           // 气压 (hPa)
    double precipitation;      // 降水量 (mm)
    int cloud_cover;           // 云量 (%)
    int uv_index;              // UV指数
    std::string condition;     // 天气状况
    std::string description;   // 详细描述
    int weather_code;          // WMO天气代码
    std::string icon_name;     // 图标名称
    int64_t timestamp;         // 时间戳
    
    // 位置信息
    std::string city;
    std::string country;
    double latitude;
    double longitude;
    std::string timezone;
    
    // 逐小时预报
    struct HourlyData {
        int64_t timestamp;
        double temperature;
        double precipitation_probability;
        int weather_code;
    };
    
    std::vector<HourlyData> hourly_forecast;
    
    // 每日预报
    struct DailyData {
        int64_t date;
        double temp_max;
        double temp_min;
        double precipitation_sum;
        int weather_code;
        std::string sunrise;
        std::string sunset;
    };
    
    std::vector<DailyData> daily_forecast;
    
    WeatherData();
};

// RPC通信数据结构
struct WeatherRequest {
    enum RequestType {
        CURRENT_WEATHER = 0,
        FORECAST = 1,
        SEARCH_CITY = 2,
        GEO_LOCATION = 3
    };
    
    RequestType type;
    std::string city_name;
    std::string country_code;
    int days;  // 预报天数
    double latitude;
    double longitude;
    std::string language;  // 语言
    std::string units;     // 单位制 (metric/imperial)
};

struct WeatherResponse {
    bool success;
    std::string error_message;
    WeatherData current_weather;
    std::vector<WeatherData> forecast;
    std::vector<std::pair<std::string, std::string>> city_suggestions; // 城市搜索建议
};

#endif // WEATHER_DATA_H