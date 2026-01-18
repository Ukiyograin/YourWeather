#include "api_client.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <memory>
#include <mutex>

using json = nlohmann::json;
using namespace std;

// HTTP客户端实现类
class APIClient::HttpClientImpl {
public:
    HttpClientImpl() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl_ = curl_easy_init();
        if (curl_) {
            curl_easy_setopt(curl_, CURLOPT_USERAGENT, "WeatherApp/1.0");
            curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 10L);
            curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 2L);
        }
    }
    
    ~HttpClientImpl() {
        if (curl_) {
            curl_easy_cleanup(curl_);
        }
        curl_global_cleanup();
    }
    
    string performRequest(const string& url) {
        if (!curl_) return "";
        
        string response;
        curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);
        
        CURLcode res = curl_easy_perform(curl_);
        if (res != CURLE_OK) {
            cerr << "HTTP请求失败: " << curl_easy_strerror(res) << endl;
            return "";
        }
        
        long http_code = 0;
        curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code != 200) {
            cerr << "HTTP错误代码: " << http_code << endl;
            return "";
        }
        
        return response;
    }
    
private:
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t total_size = size * nmemb;
        ((string*)userp)->append((char*)contents, total_size);
        return total_size;
    }
    
    CURL* curl_ = nullptr;
};

// APIClient实现
APIClient::APIClient() 
    : api_endpoint_("https://api.open-meteo.com/v1")
    , user_agent_("WeatherApp/1.0")
    , http_client_(make_unique<HttpClientImpl>()) {
}

APIClient::~APIClient() = default;

void APIClient::setEndpoint(const string& endpoint) {
    api_endpoint_ = endpoint;
}

string APIClient::buildCurrentWeatherUrl(double lat, double lon, 
                                       const string& timezone,
                                       const string& language) {
    stringstream ss;
    ss << api_endpoint_ << "/forecast?";
    ss << "latitude=" << fixed << setprecision(6) << lat;
    ss << "&longitude=" << fixed << setprecision(6) << lon;
    ss << "&current=temperature_2m,relative_humidity_2m,apparent_temperature,";
    ss << "wind_speed_10m,wind_direction_10m,pressure_msl,precipitation,";
    ss << "cloud_cover,weather_code,is_day";
    ss << "&timezone=" << timezone;
    ss << "&language=" << language;
    
    return ss.str();
}

string APIClient::buildForecastUrl(double lat, double lon, int days,
                                 const string& timezone,
                                 const string& language) {
    stringstream ss;
    ss << api_endpoint_ << "/forecast?";
    ss << "latitude=" << fixed << setprecision(6) << lat;
    ss << "&longitude=" << fixed << setprecision(6) << lon;
    ss << "&current=temperature_2m,relative_humidity_2m,apparent_temperature,";
    ss << "wind_speed_10m,wind_direction_10m,pressure_msl,precipitation,";
    ss << "cloud_cover,weather_code,is_day";
    ss << "&hourly=temperature_2m,precipitation_probability,weather_code";
    ss << "&daily=weather_code,temperature_2m_max,temperature_2m_min,";
    ss << "precipitation_sum,sunrise,sunset";
    ss << "&forecast_days=" << days;
    ss << "&timezone=" << timezone;
    ss << "&language=" << language;
    
    return ss.str();
}

string APIClient::buildGeocodingUrl(const string& query) {
    stringstream ss;
    ss << "https://geocoding-api.open-meteo.com/v1/search?";
    ss << "name=" << query;
    ss << "&count=10";
    ss << "&language=zh";
    ss << "&format=json";
    
    return ss.str();
}

string APIClient::performHttpRequest(const string& url) {
    return http_client_->performRequest(url);
}

WeatherData APIClient::parseCurrentWeatherJson(const string& json_str) {
    WeatherData data;
    
    try {
        json j = json::parse(json_str);
        
        if (j.contains("current")) {
            auto current = j["current"];
            
            data.temperature = current.value("temperature_2m", 0.0);
            data.feels_like = current.value("apparent_temperature", 0.0);
            data.humidity = current.value("relative_humidity_2m", 0);
            data.wind_speed = current.value("wind_speed_10m", 0.0);
            data.wind_direction = current.value("wind_direction_10m", 0);
            data.pressure = current.value("pressure_msl", 1013.0);
            data.precipitation = current.value("precipitation", 0.0);
            data.cloud_cover = current.value("cloud_cover", 0);
            data.weather_code = current.value("weather_code", 0);
            data.timestamp = current.value("time", 0);
            
            bool is_day = current.value("is_day", 1) == 1;
            data.icon_name = WeatherService::getIconNameFromCode(data.weather_code, is_day);
            data.condition = WeatherService::getConditionFromCode(data.weather_code);
        }
        
        if (j.contains("latitude")) {
            data.latitude = j["latitude"];
            data.longitude = j["longitude"];
        }
        
        if (j.contains("timezone")) {
            data.timezone = j["timezone"];
        }
        
    } catch (const exception& e) {
        cerr << "解析JSON错误: " << e.what() << endl;
    }
    
    return data;
}

WeatherData APIClient::parseForecastJson(const string& json_str) {
    WeatherData data = parseCurrentWeatherJson(json_str);
    
    try {
        json j = json::parse(json_str);
        
        // 解析逐小时预报
        if (j.contains("hourly")) {
            auto hourly = j["hourly"];
            
            if (hourly.contains("time") && hourly.contains("temperature_2m") &&
                hourly.contains("precipitation_probability") && hourly.contains("weather_code")) {
                
                auto times = hourly["time"];
                auto temps = hourly["temperature_2m"];
                auto precip_probs = hourly["precipitation_probability"];
                auto weather_codes = hourly["weather_code"];
                
                size_t count = min({times.size(), temps.size(), 
                                   precip_probs.size(), weather_codes.size()});
                count = min(count, static_cast<size_t>(24)); // 限制24小时
                
                for (size_t i = 0; i < count; i++) {
                    WeatherData::HourlyData hourly_data;
                    hourly_data.timestamp = times[i];
                    hourly_data.temperature = temps[i];
                    hourly_data.precipitation_probability = precip_probs[i];
                    hourly_data.weather_code = weather_codes[i];
                    data.hourly_forecast.push_back(hourly_data);
                }
            }
        }
        
        // 解析每日预报
        if (j.contains("daily")) {
            auto daily = j["daily"];
            
            if (daily.contains("time") && daily.contains("temperature_2m_max") &&
                daily.contains("temperature_2m_min") && daily.contains("precipitation_sum") &&
                daily.contains("weather_code") && daily.contains("sunrise") &&
                daily.contains("sunset")) {
                
                auto dates = daily["time"];
                auto temp_maxs = daily["temperature_2m_max"];
                auto temp_mins = daily["temperature_2m_min"];
                auto precip_sums = daily["precipitation_sum"];
                auto weather_codes = daily["weather_code"];
                auto sunrises = daily["sunrise"];
                auto sunsets = daily["sunset"];
                
                size_t count = min({dates.size(), temp_maxs.size(), temp_mins.size(),
                                   precip_sums.size(), weather_codes.size(),
                                   sunrises.size(), sunsets.size()});
                
                for (size_t i = 0; i < count; i++) {
                    WeatherData::DailyData daily_data;
                    daily_data.date = dates[i];
                    daily_data.temp_max = temp_maxs[i];
                    daily_data.temp_min = temp_mins[i];
                    daily_data.precipitation_sum = precip_sums[i];
                    daily_data.weather_code = weather_codes[i];
                    daily_data.sunrise = sunrises[i];
                    daily_data.sunset = sunsets[i];
                    data.daily_forecast.push_back(daily_data);
                }
            }
        }
        
    } catch (const exception& e) {
        cerr << "解析预报JSON错误: " << e.what() << endl;
    }
    
    return data;
}

WeatherData APIClient::getCurrentWeather(double lat, double lon, 
                                       const string& timezone,
                                       const string& language) {
    string url = buildCurrentWeatherUrl(lat, lon, timezone, language);
    string json_str = performHttpRequest(url);
    return parseCurrentWeatherJson(json_str);
}

WeatherData APIClient::getForecast(double lat, double lon, 
                                 int days,
                                 const string& timezone,
                                 const string& language) {
    string url = buildForecastUrl(lat, lon, days, timezone, language);
    string json_str = performHttpRequest(url);
    return parseForecastJson(json_str);
}

pair<double, double> APIClient::getCoordinates(const string& city, 
                                             const string& country) {
    string query = city;
    if (!country.empty()) {
        query += "," + country;
    }
    
    string url = buildGeocodingUrl(query);
    string json_str = performHttpRequest(url);
    
    try {
        json j = json::parse(json_str);
        
        if (j.contains("results") && j["results"].size() > 0) {
            auto result = j["results"][0];
            double lat = result["latitude"];
            double lon = result["longitude"];
            return {lat, lon};
        }
    } catch (const exception& e) {
        cerr << "解析地理编码JSON错误: " << e.what() << endl;
    }
    
    return {0.0, 0.0};
}

vector<pair<string, string>> APIClient::searchCity(const string& query, int limit) {
    vector<pair<string, string>> results;
    
    string url = buildGeocodingUrl(query);
    string json_str = performHttpRequest(url);
    
    try {
        json j = json::parse(json_str);
        
        if (j.contains("results")) {
            int count = min(static_cast<int>(j["results"].size()), limit);
            
            for (int i = 0; i < count; i++) {
                auto result = j["results"][i];
                string name = result["name"];
                string country = result["country"];
                results.push_back({name, country});
            }
        }
    } catch (const exception& e) {
        cerr << "解析城市搜索JSON错误: " << e.what() << endl;
    }
    
    return results;
}