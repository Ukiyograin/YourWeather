#include "weather_service.h"
#include "icon_renderer.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <memory>
#include <csignal>
#include <cstdlib>
#include <atomic>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

using namespace std;
using namespace chrono;

atomic<bool> running{true};

// 信号处理
#ifdef _WIN32
BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT) {
        running = false;
        return TRUE;
    }
    return FALSE;
}
#else
void SignalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        running = false;
    }
}
#endif

// 创建守护进程（Linux/macOS）
#ifdef __linux__
void Daemonize() {
    pid_t pid = fork();
    
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    
    if (setsid() < 0) {
        exit(EXIT_FAILURE);
    }
    
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    
    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    
    umask(0);
    
    chdir("/");
    
    // 关闭所有打开的文件描述符
    for (int x = sysconf(_SC_OPEN_MAX); x >= 0; x--) {
        close(x);
    }
}
#endif

// 简单的HTTP服务器（用于演示，实际项目应该使用cpp-httplib或类似库）
class SimpleHttpServer {
public:
    SimpleHttpServer(int port) : port_(port) {}
    
    void Start() {
        // 这里应该启动一个真正的HTTP服务器
        // 为了简化，我们只打印信息
        cout << "HTTP服务器监听在端口 " << port_ << endl;
        cout << "API端点:" << endl;
        cout << "  GET /api/weather?city=北京" << endl;
        cout << "  GET /api/forecast?city=北京&days=7" << endl;
        cout << "  GET /api/search?q=Bei" << endl;
        cout << "  GET /api/icon/sunny.png" << endl;
    }
    
    void Stop() {
        cout << "HTTP服务器已停止" << endl;
    }
    
private:
    int port_;
};

// 配置文件管理
class ConfigManager {
public:
    struct Config {
        string api_endpoint = "https://api.open-meteo.com/v1";
        string language = "zh";
        string units = "metric";
        int cache_ttl = 300; // 5分钟
        int http_port = 8080;
        bool daemon_mode = false;
        bool enable_cache = true;
        string log_file = "weather_service.log";
    };
    
    Config LoadConfig(const string& filename) {
        Config config;
        
        // 简化的配置加载
        // 实际项目中应该使用JSON或YAML解析库
        ifstream file(filename);
        if (file.is_open()) {
            string line;
            while (getline(file, line)) {
                size_t pos = line.find('=');
                if (pos != string::npos) {
                    string key = line.substr(0, pos);
                    string value = line.substr(pos + 1);
                    
                    if (key == "api_endpoint") config.api_endpoint = value;
                    else if (key == "language") config.language = value;
                    else if (key == "units") config.units = value;
                    else if (key == "cache_ttl") config.cache_ttl = stoi(value);
                    else if (key == "http_port") config.http_port = stoi(value);
                    else if (key == "daemon_mode") config.daemon_mode = (value == "true");
                    else if (key == "enable_cache") config.enable_cache = (value == "true");
                    else if (key == "log_file") config.log_file = value;
                }
            }
        }
        
        return config;
    }
    
    void SaveConfig(const Config& config, const string& filename) {
        ofstream file(filename);
        if (file.is_open()) {
            file << "api_endpoint=" << config.api_endpoint << endl;
            file << "language=" << config.language << endl;
            file << "units=" << config.units << endl;
            file << "cache_ttl=" << config.cache_ttl << endl;
            file << "http_port=" << config.http_port << endl;
            file << "daemon_mode=" << (config.daemon_mode ? "true" : "false") << endl;
            file << "enable_cache=" << (config.enable_cache ? "true" : "false") << endl;
            file << "log_file=" << config.log_file << endl;
        }
    }
};

// 日志系统
class Logger {
public:
    enum LogLevel {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };
    
    Logger(const string& filename = "") {
        if (!filename.empty()) {
            log_file_.open(filename, ios::app);
        }
    }
    
    ~Logger() {
        if (log_file_.is_open()) {
            log_file_.close();
        }
    }
    
    void Log(LogLevel level, const string& message) {
        string timestamp = GetCurrentTime();
        string levelStr = GetLevelString(level);
        
        string logEntry = timestamp + " [" + levelStr + "] " + message;
        
        // 输出到控制台
        cout << logEntry << endl;
        
        // 输出到文件
        if (log_file_.is_open()) {
            log_file_ << logEntry << endl;
        }
    }
    
private:
    ofstream log_file_;
    
    string GetCurrentTime() {
        auto now = system_clock::now();
        auto time = system_clock::to_time_t(now);
        auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
        
        stringstream ss;
        ss << put_time(localtime(&time), "%Y-%m-%d %H:%M:%S");
        ss << '.' << setfill('0') << setw(3) << ms.count();
        
        return ss.str();
    }
    
    string GetLevelString(LogLevel level) {
        switch (level) {
            case DEBUG: return "DEBUG";
            case INFO: return "INFO";
            case WARNING: return "WARN";
            case ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }
};

// 主程序
int main(int argc, char* argv[]) {
    cout << "==========================================" << endl;
    cout << "       天气服务后端 v1.0.0" << endl;
    cout << "==========================================" << endl;
    cout << endl;
    
    // 设置信号处理
#ifdef _WIN32
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
#else
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);
#endif
    
    // 加载配置
    ConfigManager configManager;
    string configFile = "weather_service.conf";
    
    if (argc > 1) {
        configFile = argv[1];
    }
    
    auto config = configManager.LoadConfig(configFile);
    
    // 设置日志
    Logger logger(config.log_file);
    logger.Log(Logger::INFO, "启动天气服务...");
    
    // 守护进程模式
    if (config.daemon_mode) {
#ifdef __linux__
        logger.Log(Logger::INFO, "进入守护进程模式...");
        Daemonize();
#else
        logger.Log(Logger::WARNING, "守护进程模式只在Linux系统支持");
#endif
    }
    
    // 创建天气服务
    unique_ptr<WeatherService> weatherService;
    
    try {
        weatherService = make_unique<WeatherService>();
        if (!weatherService->initialize()) {
            logger.Log(Logger::ERROR, "初始化天气服务失败");
            return 1;
        }
        
        weatherService->setCacheEnabled(config.enable_cache);
        weatherService->setCacheTTL(config.cache_ttl);
        weatherService->setLanguage(config.language);
        weatherService->setUnits(config.units);
        
        logger.Log(Logger::INFO, "天气服务初始化成功");
    } catch (const exception& e) {
        logger.Log(Logger::ERROR, string("初始化失败: ") + e.what());
        return 1;
    }
    
    // 创建图标渲染器
    unique_ptr<IconRenderer> iconRenderer;
    try {
        iconRenderer = make_unique<IconRenderer>();
        logger.Log(Logger::INFO, "图标渲染器初始化成功");
    } catch (const exception& e) {
        logger.Log(Logger::WARNING, string("图标渲染器初始化失败: ") + e.what());
    }
    
    // 启动HTTP服务器
    SimpleHttpServer httpServer(config.http_port);
    thread httpThread([&httpServer]() {
        httpServer.Start();
    });
    
    // 显示帮助信息
    cout << endl;
    cout << "服务已启动，输入命令控制服务：" << endl;
    cout << "  stats        - 显示统计信息" << endl;
    cout << "  test <城市>  - 测试天气查询" << endl;
    cout << "  icon <名称>  - 测试图标渲染" << endl;
    cout << "  config       - 显示当前配置" << endl;
    cout << "  clear        - 清空缓存" << endl;
    cout << "  save         - 保存配置" << endl;
    cout << "  help         - 显示帮助" << endl;
    cout << "  quit/exit    - 退出程序" << endl;
    cout << endl;
    
    // 主命令循环
    while (running) {
        cout << "> ";
        
        string command;
        if (!getline(cin, command)) {
            // 输入结束
            break;
        }
        
        // 处理命令
        if (command == "quit" || command == "exit") {
            break;
        } else if (command == "stats") {
            if (weatherService) {
                auto stats = weatherService->getStatistics();
                cout << "统计信息:" << endl;
                cout << "  总请求数: " << stats.total_requests << endl;
                cout << "  缓存命中: " << stats.cache_hits << endl;
                cout << "  API调用: " << stats.api_calls << endl;
                cout << "  缓存命中率: " 
                     << (stats.total_requests > 0 ? 
                         (stats.cache_hits * 100.0 / stats.total_requests) : 0)
                     << "%" << endl;
                cout << "  平均响应时间: " 
                     << (stats.total_requests > 0 ? 
                         stats.total_response_time / stats.total_requests : 0)
                     << "ms" << endl;
            }
        } else if (command.find("test ") == 0) {
            string city = command.substr(5);
            if (!city.empty()) {
                cout << "测试查询城市: " << city << endl;
                
                try {
                    WeatherRequest request;
                    request.type = WeatherRequest::CURRENT_WEATHER;
                    request.city_name = city;
                    request.language = config.language;
                    
                    auto response = weatherService->processRequest(request);
                    
                    if (response.success) {
                        cout << "查询成功:" << endl;
                        cout << "  城市: " << response.current_weather.city << endl;
                        cout << "  温度: " << response.current_weather.temperature << "°C" << endl;
                        cout << "  体感温度: " << response.current_weather.feels_like << "°C" << endl;
                        cout << "  湿度: " << response.current_weather.humidity << "%" << endl;
                        cout << "  风速: " << response.current_weather.wind_speed << " km/h" << endl;
                        cout << "  天气状况: " << response.current_weather.condition << endl;
                        cout << "  图标: " << response.current_weather.icon_name << endl;
                    } else {
                        cout << "查询失败: " << response.error_message << endl;
                    }
                } catch (const exception& e) {
                    cout << "查询异常: " << e.what() << endl;
                }
            }
        } else if (command.find("icon ") == 0) {
            if (iconRenderer) {
                string iconName = command.substr(5);
                cout << "测试渲染图标: " << iconName << endl;
                
                try {
                    auto iconData = iconRenderer->renderIcon(iconName, IconRenderer::LARGE);
                    cout << "图标渲染成功，大小: " << iconData.size() << " 字节" << endl;
                    
                    // 保存到文件
                    string filename = iconName + ".bmp";
                    if (iconRenderer->renderIconToFile(iconName, filename)) {
                        cout << "图标已保存到: " << filename << endl;
                    }
                    
                    // 获取SVG路径数据
                    string svgPath = iconRenderer->getSVGPathData(iconName);
                    if (!svgPath.empty()) {
                        cout << "SVG路径数据: " << svgPath << endl;
                    }
                } catch (const exception& e) {
                    cout << "图标渲染失败: " << e.what() << endl;
                }
            } else {
                cout << "图标渲染器未初始化" << endl;
            }
        } else if (command == "config") {
            cout << "当前配置:" << endl;
            cout << "  API端点: " << config.api_endpoint << endl;
            cout << "  语言: " << config.language << endl;
            cout << "  单位制: " << config.units << endl;
            cout << "  缓存TTL: " << config.cache_ttl << "秒" << endl;
            cout << "  HTTP端口: " << config.http_port << endl;
            cout << "  守护进程模式: " << (config.daemon_mode ? "是" : "否") << endl;
            cout << "  启用缓存: " << (config.enable_cache ? "是" : "否") << endl;
            cout << "  日志文件: " << config.log_file << endl;
        } else if (command == "clear") {
            if (weatherService) {
                weatherService->setCacheEnabled(false);
                weatherService->setCacheEnabled(true);
                cout << "缓存已清空" << endl;
            }
        } else if (command == "save") {
            configManager.SaveConfig(config, configFile);
            cout << "配置已保存到: " << configFile << endl;
        } else if (command == "help") {
            cout << "可用命令:" << endl;
            cout << "  stats                 - 显示统计信息" << endl;
            cout << "  test <城市>           - 测试天气查询" << endl;
            cout << "  icon <名称>           - 测试图标渲染" << endl;
            cout << "  config                - 显示当前配置" << endl;
            cout << "  clear                 - 清空缓存" << endl;
            cout << "  save                  - 保存配置" << endl;
            cout << "  help                  - 显示帮助" << endl;
            cout << "  quit/exit             - 退出程序" << endl;
        } else if (!command.empty()) {
            cout << "未知命令: " << command << endl;
            cout << "输入 'help' 查看可用命令" << endl;
        }
        
        // 防止CPU占用过高
        this_thread::sleep_for(milliseconds(10));
    }
    
    // 停止服务
    logger.Log(Logger::INFO, "正在停止服务...");
    
    running = false;
    
    // 停止HTTP服务器
    httpServer.Stop();
    if (httpThread.joinable()) {
        httpThread.join();
    }
    
    logger.Log(Logger::INFO, "服务已停止");
    cout << "再见！" << endl;
    
    return 0;
}