#include "weather_service.h"
#include "api_client.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <ctime>

using namespace std;

// WeatherCache实现
WeatherCache::WeatherCache(int64_t default_ttl) 
    : default_ttl_(default_ttl) {
}

void WeatherCache::put(const string& key, const WeatherData& data, int64_t ttl) {
    lock_guard<mutex> lock(mutex_);
    
    int64_t now = time(nullptr);
    CacheEntry entry;
    entry.data = data;
    entry.timestamp = now;
    entry.expiry = now + (ttl > 0 ? ttl : default_ttl_);
    
    cache_[key] = entry;
}

bool WeatherCache::get(const string& key, WeatherData& data) {
    lock_guard<mutex> lock(mutex_);
    
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        int64_t now = time(nullptr);
        if (now < it->second.expiry) {
            data = it->second.data;
            return true;
        } else {
            cache_.erase(it);
        }
    }
    
    return false;
}

void WeatherCache::clear() {
    lock_guard<mutex> lock(mutex_);
    cache_.clear();
}

void WeatherCache::cleanup() {
    lock_guard<mutex> lock(mutex_);
    int64_t now = time(nullptr);
    
    for (auto it = cache_.begin(); it != cache_.end();) {
        if (now >= it->second.expiry) {
            it = cache_.erase(it);
        } else {
            ++it;
        }
    }
}

// WeatherService实现
WeatherService::WeatherService() 
    : cache_enabled_(true)
    , language_("zh")
    , units_("metric") {
    
    api_client_ = make_unique<APIClient>();
    cache_ = make_unique<WeatherCache>(300); // 5分钟缓存
    
    stats_.total_requests = 0;
    stats_.cache_hits = 0;
    stats_.api_calls = 0;
    stats_.total_response_time = 0;
}

WeatherService::~WeatherService() = default;

bool WeatherService::initialize() {
    // 初始化API客户端
    api_client_->setEndpoint("https://api.open-meteo.com/v1");
    
    // 清理过期缓存
    cache_->cleanup();
    
    return true;
}

string WeatherService::getConditionFromCode(int code, const string& language) {
    // WMO天气代码翻译
    static const unordered_map<int, string> zh_cn = {
        {0, "晴天"},
        {1, "大部晴朗"},
        {2, "部分多云"},
        {3, "阴天"},
        {45, "有雾"},
        {48, "有雾"},
        {51, "小雨"},
        {53, "中雨"},
        {55, "大雨"},
        {56, "冻毛毛雨"},
        {57, "强冻毛毛雨"},
        {61, "小雨"},
        {63, "中雨"},
        {65, "大雨"},
        {66, "冻雨"},
        {67, "强冻雨"},
        {71, "小雪"},
        {73, "中雪"},
        {75, "大雪"},
        {77, "雪粒"},
        {80, "小阵雨"},
        {81, "中阵雨"},
        {82, "强阵雨"},
        {85, "小阵雪"},
        {86, "大阵雪"},
        {95, "雷暴"},
        {96, "小雹雷暴"},
        {99, "大雹雷暴"}
    };
    
    static const unordered_map<int, string> en = {
        {0, "Clear sky"},
        {1, "Mainly clear"},
        {2, "Partly cloudy"},
        {3, "Overcast"},
        {45, "Fog"},
        {48, "Fog"},
        {51, "Light drizzle"},
        {53, "Moderate drizzle"},
        {55, "Dense drizzle"},
        {56, "Light freezing drizzle"},
        {57, "Dense freezing drizzle"},
        {61, "Slight rain"},
        {63, "Moderate rain"},
        {65, "Heavy rain"},
        {66, "Light freezing rain"},
        {67, "Heavy freezing rain"},
        {71, "Slight snow fall"},
        {73, "Moderate snow fall"},
        {75, "Heavy snow fall"},
        {77, "Snow grains"},
        {80, "Slight rain showers"},
        {81, "Moderate rain showers"},
        {82, "Violent rain showers"},
        {85, "Slight snow showers"},
        {86, "Heavy snow showers"},
        {95, "Thunderstorm"},
        {96, "Thunderstorm with slight hail"},
        {99, "Thunderstorm with heavy hail"}
    };
    
    if (language == "zh" || language == "zh-CN") {
        auto it = zh_cn.find(code);
        return it != zh_cn.end() ? it->second : "未知";
    } else {
        auto it = en.find(code);
        return it != en.end() ? it->second : "Unknown";
    }
}

string WeatherService::getIconNameFromCode(int code, bool is_day) {
    // 根据WMO天气代码和白天/夜晚返回图标名称
    if (code == 0) return is_day ? "sunny" : "clear-night";
    if (code >= 1 && code <= 3) return is_day ? "partly-cloudy-day" : "partly-cloudy-night";
    if (code == 45 || code == 48) return "fog";
    if (code >= 51 && code <= 57) return "drizzle";
    if (code >= 61 && code <= 67) return "rain";
    if (code >= 71 && code <= 77) return "snow";
    if (code >= 80 && code <= 82) return "rain";
    if (code >= 85 && code <= 86) return "snow";
    if (code >= 95 && code <= 99) return "thunderstorm";
    
    return "unknown";
}

string WeatherService::formatTemperature(double temp, const string& units) {
    stringstream ss;
    ss << fixed << setprecision(1) << temp;
    
    if (units == "metric") {
        ss << "°C";
    } else if (units == "imperial") {
        ss << "°F";
    }
    
    return ss.str();
}

string WeatherService::formatWindSpeed(double speed, const string& units) {
    stringstream ss;
    ss << fixed << setprecision(1) << speed;
    
    if (units == "metric") {
        ss << " km/h";
    } else if (units == "imperial") {
        ss << " mph";
    }
    
    return ss.str();
}

string WeatherService::formatPressure(double pressure) {
    stringstream ss;
    ss << fixed << setprecision(0) << pressure << " hPa";
    return ss.str();
}

WeatherResponse WeatherService::processRequest(const WeatherRequest& request) {
    auto start_time = chrono::high_resolution_clock::now();
    
    {
        lock_guard<mutex> lock(stats_mutex_);
        stats_.total_requests++;
    }
    
    WeatherResponse response;
    response.success = false;
    
    try {
        switch (request.type) {
            case WeatherRequest::CURRENT_WEATHER:
                response = handleCurrentWeather(request);
                break;
            case WeatherRequest::FORECAST:
                response = handleForecast(request);
                break;
            case WeatherRequest::SEARCH_CITY:
                response = handleCitySearch(request);
                break;
            case WeatherRequest::GEO_LOCATION:
                response = handleGeoLocation(request);
                break;
            default:
                response.error_message = "未知请求类型";
                break;
        }
    } catch (const exception& e) {
        response.error_message = string("处理请求时出错: ") + e.what();
    }
    
    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    
    {
        lock_guard<mutex> lock(stats_mutex_);
        stats_.total_response_time += duration.count();
    }
    
    return response;
}

WeatherResponse WeatherService::handleCurrentWeather(const WeatherRequest& request) {
    WeatherResponse response;
    
    // 生成缓存键
    string cache_key = "current_" + request.city_name + "_" + request.country_code;
    
    // 尝试从缓存获取
    if (cache_enabled_) {
        WeatherData cached_data;
        if (cache_->get(cache_key, cached_data)) {
            {
                lock_guard<mutex> lock(stats_mutex_);
                stats_.cache_hits++;
            }
            response.current_weather = cached_data;
            response.success = true;
            return response;
        }
    }
    
    // 从API获取
    auto coords = getCityCoordinates(request.city_name, request.country_code);
    if (coords.first == 0.0 && coords.second == 0.0) {
        response.error_message = "无法找到城市坐标";
        return response;
    }
    
    WeatherData weather = api_client_->getCurrentWeather(
        coords.first, coords.second, "auto", request.language.empty() ? language_ : request.language);
    
    weather.city = request.city_name;
    weather.country = request.country_code;
    weather.latitude = coords.first;
    weather.longitude = coords.second;
    
    // 缓存结果
    if (cache_enabled_) {
        cache_->put(cache_key, weather);
    }
    
    {
        lock_guard<mutex> lock(stats_mutex_);
        stats_.api_calls++;
    }
    
    response.current_weather = weather;
    response.success = true;
    
    return response;
}

WeatherResponse WeatherService::handleForecast(const WeatherRequest& request) {
    WeatherResponse response;
    
    string cache_key = "forecast_" + request.city_name + "_" + 
                      request.country_code + "_" + to_string(request.days);
    
    if (cache_enabled_) {
        // 这里简化处理，实际应该缓存整个预报
    }
    
    auto coords = getCityCoordinates(request.city_name, request.country_code);
    if (coords.first == 0.0 && coords.second == 0.0) {
        response.error_message = "无法找到城市坐标";
        return response;
    }
    
    int days = request.days > 0 ? request.days : 3;
    WeatherData weather = api_client_->getForecast(
        coords.first, coords.second, days, "auto", 
        request.language.empty() ? language_ : request.language);
    
    weather.city = request.city_name;
    weather.country = request.country_code;
    
    // 提取每日预报到响应中
    for (size_t i = 0; i < weather.daily_forecast.size(); i++) {
        WeatherData daily;
        daily.temperature = weather.daily_forecast[i].temp_max;
        daily.weather_code = weather.daily_forecast[i].weather_code;
        daily.icon_name = getIconNameFromCode(daily.weather_code, true);
        daily.condition = getConditionFromCode(daily.weather_code, language_);
        response.forecast.push_back(daily);
    }
    
    response.current_weather = weather;
    response.success = true;
    
    {
        lock_guard<mutex> lock(stats_mutex_);
        stats_.api_calls++;
    }
    
    return response;
}

WeatherResponse WeatherService::handleCitySearch(const WeatherRequest& request) {
    WeatherResponse response;
    
    if (request.city_name.empty()) {
        response.error_message = "搜索查询不能为空";
        return response;
    }
    
    auto results = api_client_->searchCity(request.city_name, 10);
    response.city_suggestions = results;
    response.success = !results.empty();
    
    if (!response.success) {
        response.error_message = "未找到匹配的城市";
    }
    
    return response;
}

WeatherResponse WeatherService::handleGeoLocation(const WeatherRequest& request) {
    WeatherResponse response;
    
    if (request.latitude == 0.0 && request.longitude == 0.0) {
        response.error_message = "无效的经纬度";
        return response;
    }
    
    WeatherData weather = api_client_->getCurrentWeather(
        request.latitude, request.longitude, "auto", language_);
    
    response.current_weather = weather;
    response.success = true;
    
    {
        lock_guard<mutex> lock(stats_mutex_);
        stats_.api_calls++;
    }
    
    return response;
}

pair<double, double> WeatherService::getCityCoordinates(const string& city, 
                                                      const string& country) {
    return api_client_->getCoordinates(city, country);
}

void WeatherService::setCacheEnabled(bool enabled) {
    cache_enabled_ = enabled;
}

void WeatherService::setCacheTTL(int64_t ttl_seconds) {
    cache_ = make_unique<WeatherCache>(ttl_seconds);
}

void WeatherService::setLanguage(const string& language) {
    language_ = language;
}

void WeatherService::setUnits(const string& units) {
    units_ = units;
}

WeatherService::Statistics WeatherService::getStatistics() const {
    lock_guard<mutex> lock(stats_mutex_);
    return stats_;
}