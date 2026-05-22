#include "weather_service.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>

using json = nlohmann::json;

static void printUsage() {
    std::cerr << "用法:\n"
              << "  weather_service_backend current <city>\n"
              << "  weather_service_backend forecast <city> [days]\n"
              << "  weather_service_backend search <query>\n";
}

static json weatherToJson(const WeatherData& weather) {
    json hourly = json::array();
    for (const auto& item : weather.hourly_forecast) {
        hourly.push_back({
            {"time", item.time},
            {"temperature", item.temperature},
            {"precipitationProbability", item.precipitation_probability},
            {"weatherCode", item.weather_code},
            {"condition", WeatherService::getConditionFromCode(item.weather_code)},
            {"iconName", WeatherService::getIconNameFromCode(item.weather_code, true)}
        });
    }

    json daily = json::array();
    for (const auto& item : weather.daily_forecast) {
        daily.push_back({
            {"date", item.date},
            {"tempMax", item.temp_max},
            {"tempMin", item.temp_min},
            {"precipitation", item.precipitation_sum},
            {"weatherCode", item.weather_code},
            {"condition", WeatherService::getConditionFromCode(item.weather_code)},
            {"iconName", WeatherService::getIconNameFromCode(item.weather_code, true)},
            {"sunrise", item.sunrise},
            {"sunset", item.sunset}
        });
    }

    return {
        {"city", weather.city},
        {"country", weather.country},
        {"latitude", weather.latitude},
        {"longitude", weather.longitude},
        {"timezone", weather.timezone},
        {"temperature", weather.temperature},
        {"feelsLike", weather.feels_like},
        {"humidity", weather.humidity},
        {"windSpeed", weather.wind_speed},
        {"windDirection", weather.wind_direction},
        {"pressure", weather.pressure},
        {"precipitation", weather.precipitation},
        {"uvIndex", weather.uv_index},
        {"condition", weather.condition},
        {"description", weather.description},
        {"weatherCode", weather.weather_code},
        {"iconName", weather.icon_name},
        {"time", weather.time},
        {"hourlyForecast", hourly},
        {"dailyForecast", daily}
    };
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printUsage();
        return 2;
    }

    WeatherService service;
    if (!service.initialize()) {
        std::cerr << "天气服务初始化失败\n";
        return 1;
    }

    std::string command = argv[1];
    WeatherRequest request;
    request.city_name = argv[2];
    request.language = "zh";

    if (command == "current") {
        request.type = WeatherRequest::CURRENT_WEATHER;
    } else if (command == "forecast") {
        request.type = WeatherRequest::FORECAST;
        request.days = argc >= 4 ? std::stoi(argv[3]) : 7;
    } else if (command == "search") {
        request.type = WeatherRequest::SEARCH_CITY;
    } else {
        printUsage();
        return 2;
    }

    auto response = service.processRequest(request);
    if (!response.success) {
        std::cout << json({{"success", false}, {"error", response.error_message}}).dump(2) << std::endl;
        return 1;
    }

    if (command == "search") {
        json results = json::array();
        for (const auto& city : response.city_suggestions) {
            results.push_back({{"name", city.first}, {"country", city.second}});
        }
        std::cout << json({{"success", true}, {"results", results}}).dump(2) << std::endl;
        return 0;
    }

    std::cout << json({{"success", true}, {"weather", weatherToJson(response.current_weather)}}).dump(2) << std::endl;
    return 0;
}
