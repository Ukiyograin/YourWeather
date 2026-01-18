#ifndef WEATHER_SERVICE_H
#define WEATHER_SERVICE_H

#include "weather_data.h"
#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>

class WeatherCache {
public:
    struct CacheEntry {
        WeatherData data;
        int64_t timestamp;
        int64_t expiry;
    };
    
    WeatherCache(int64_t default_ttl = 300); // 5分钟默认缓存时间
    
    void put(const std::string& key, const WeatherData& data, int64_t ttl = 0);
    bool get(const std::string& key, WeatherData& data);
    void clear();
    void cleanup(); // 清理过期缓存
    
private:
    std::unordered_map<std::string, CacheEntry> cache_;
    std::mutex mutex_;
    int64_t default_ttl_;
};

class WeatherService {
public:
    WeatherService();
    ~WeatherService();
    
    // 初始化服务
    bool initialize();
    
    // 主要API接口
    WeatherResponse processRequest(const WeatherRequest& request);
    
    // 工具函数
    static std::string getConditionFromCode(int code, const std::string& language = "zh");
    static std::string getIconNameFromCode(int code, bool is_day = true);
    static std::string formatTemperature(double temp, const std::string& units = "metric");
    static std::string formatWindSpeed(double speed, const std::string& units = "metric");
    static std::string formatPressure(double pressure);
    
    // 配置
    void setCacheEnabled(bool enabled);
    void setCacheTTL(int64_t ttl_seconds);
    void setLanguage(const std::string& language);
    void setUnits(const std::string& units);
    
    // 统计信息
    struct Statistics {
        int total_requests;
        int cache_hits;
        int api_calls;
        int64_t total_response_time;
    };
    
    Statistics getStatistics() const;
    
private:
    WeatherResponse handleCurrentWeather(const WeatherRequest& request);
    WeatherResponse handleForecast(const WeatherRequest& request);
    WeatherResponse handleCitySearch(const WeatherRequest& request);
    WeatherResponse handleGeoLocation(const WeatherRequest& request);
    
    // 内部方法
    std::pair<double, double> getCityCoordinates(const std::string& city, 
                                                 const std::string& country);
    
    std::unique_ptr<APIClient> api_client_;
    std::unique_ptr<WeatherCache> cache_;
    
    bool cache_enabled_;
    std::string language_;
    std::string units_;
    
    mutable std::mutex stats_mutex_;
    Statistics stats_;
    
    // RPC服务器
    class RPCServerImpl;
    std::unique_ptr<RPCServerImpl> rpc_server_;
};

#endif // WEATHER_SERVICE_H