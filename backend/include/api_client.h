#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <string>
#include <memory>
#include "weather_data.h"

class APIClient {
public:
    APIClient();
    ~APIClient();
    
    // 设置API端点
    void setEndpoint(const std::string& endpoint);
    
    // 获取当前天气
    WeatherData getCurrentWeather(double lat, double lon, 
                                  const std::string& timezone = "auto",
                                  const std::string& language = "zh");
    
    // 获取天气预报
    WeatherData getForecast(double lat, double lon, 
                            int days = 7,
                            const std::string& timezone = "auto",
                            const std::string& language = "zh");
    
    // 根据城市名获取坐标
    std::pair<double, double> getCoordinates(const std::string& city, 
                                             const std::string& country = "");
    
    // 搜索城市
    std::vector<std::pair<std::string, std::string>> 
    searchCity(const std::string& query, int limit = 10);
    
private:
    std::string buildCurrentWeatherUrl(double lat, double lon, 
                                       const std::string& timezone,
                                       const std::string& language);
    
    std::string buildForecastUrl(double lat, double lon, int days,
                                 const std::string& timezone,
                                 const std::string& language);
    
    std::string buildGeocodingUrl(const std::string& query);
    
    std::string performHttpRequest(const std::string& url);
    
    WeatherData parseCurrentWeatherJson(const std::string& json);
    WeatherData parseForecastJson(const std::string& json);
    
    std::string api_endpoint_;
    std::string user_agent_;
    
    // HTTP客户端实现
    class HttpClientImpl;
    std::unique_ptr<HttpClientImpl> http_client_;
};

#endif // API_CLIENT_H