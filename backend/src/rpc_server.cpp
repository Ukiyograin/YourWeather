#include "weather_service.h"
#include <grpcpp/grpcpp.h>
#include <thread>
#include <memory>
#include <mutex>

using namespace grpc;
using namespace std;

// Protobuf定义（简化，实际应该用.proto文件）
class WeatherRPC : public Service {
public:
    explicit WeatherRPC(shared_ptr<WeatherService> service) 
        : weather_service_(service) {
    }
    
    // RPC方法实现
    Status GetCurrentWeather(ServerContext* context, 
                           const WeatherRequest* request,
                           WeatherResponse* response) override {
        *response = weather_service_->processRequest(*request);
        return Status::OK;
    }
    
    Status GetForecast(ServerContext* context,
                      const WeatherRequest* request,
                      ServerWriter<WeatherResponse>* writer) override {
        // 流式返回预报数据
        WeatherResponse response = weather_service_->processRequest(*request);
        writer->Write(response);
        
        // 可以添加更多预报数据...
        return Status::OK;
    }
    
private:
    shared_ptr<WeatherService> weather_service_;
};

class RPCServer {
public:
    RPCServer(const string& address, shared_ptr<WeatherService> service)
        : address_(address), weather_service_(service) {
    }
    
    void Start() {
        ServerBuilder builder;
        builder.AddListeningPort(address_, InsecureServerCredentials());
        
        auto rpc_service = make_shared<WeatherRPC>(weather_service_);
        builder.RegisterService(rpc_service.get());
        
        server_ = builder.BuildAndStart();
        cout << "RPC服务器监听在 " << address_ << endl;
        
        server_->Wait();
    }
    
    void Stop() {
        if (server_) {
            server_->Shutdown();
        }
    }
    
private:
    string address_;
    shared_ptr<WeatherService> weather_service_;
    unique_ptr<Server> server_;
};

// 主程序入口
int main(int argc, char* argv[]) {
    cout << "启动天气服务后端..." << endl;
    
    auto weather_service = make_shared<WeatherService>();
    if (!weather_service->initialize()) {
        cerr << "初始化天气服务失败" << endl;
        return 1;
    }
    
    cout << "天气服务初始化成功" << endl;
    
    // 启动RPC服务器
    RPCServer rpc_server("0.0.0.0:50051", weather_service);
    
    thread rpc_thread([&rpc_server]() {
        rpc_server.Start();
    });
    
    cout << "输入 'quit' 或 'exit' 停止服务" << endl;
    
    // 主循环
    string command;
    while (true) {
        cout << "> ";
        getline(cin, command);
        
        if (command == "quit" || command == "exit") {
            break;
        } else if (command == "stats") {
            auto stats = weather_service->getStatistics();
            cout << "统计信息:" << endl;
            cout << "  总请求数: " << stats.total_requests << endl;
            cout << "  缓存命中: " << stats.cache_hits << endl;
            cout << "  API调用: " << stats.api_calls << endl;
            cout << "  平均响应时间: " 
                 << (stats.total_requests > 0 ? 
                     stats.total_response_time / stats.total_requests : 0)
                 << "ms" << endl;
        } else if (command == "clear cache") {
            // 清空缓存
            weather_service->setCacheEnabled(false);
            weather_service->setCacheEnabled(true);
            cout << "缓存已清空" << endl;
        } else if (command == "help") {
            cout << "可用命令:" << endl;
            cout << "  stats        - 显示统计信息" << endl;
            cout << "  clear cache  - 清空缓存" << endl;
            cout << "  quit/exit    - 退出程序" << endl;
        } else {
            cout << "未知命令，输入 'help' 查看帮助" << endl;
        }
    }
    
    cout << "正在停止服务..." << endl;
    rpc_server.Stop();
    rpc_thread.join();
    
    cout << "服务已停止" << endl;
    return 0;
}