# YourWeather | 你的天气

## 项目简介

这是一个跨平台的桌面天气应用，使用C++和C#开发。后端服务使用C++处理天气数据获取和缓存，前端使用C# WPF提供美观的用户界面。应用通过Open-Meteo API获取实时天气数据，提供了完整的天气信息展示和预报功能。

## 技术架构

### 后端服务 (C++)
- 开发语言：C++ 17
- HTTP客户端：libcurl
- JSON解析：nlohmann/json
- 数据缓存：自定义缓存机制
- 构建系统：CMake

### 前端界面 (C#)
- 开发框架：WPF (.NET 6.0)
- 设计模式：MVVM
- 数据绑定：双向数据绑定
- 矢量图形：自定义图标渲染

### 数据源
- 天气API：Open-Meteo
- 地理位置API：Open-Meteo Geocoding
- 数据格式：JSON

## 功能特性

### 天气信息展示
- 当前温度、体感温度
- 湿度、风速、风向
- 气压、降水量、紫外线指数
- 天气状况描述
- 矢量天气图标

### 天气预报
- 24小时逐小时预报
- 7天每日预报
- 温度变化趋势
- 降水概率预测
- 日出日落时间

### 城市管理
- 全球城市搜索
- 最近访问记录
- 手动输入城市
- 自动定位（需系统支持）

### 用户体验
- 响应式界面设计
- 白天/夜晚主题
- 温度单位切换（摄氏度/华氏度）
- 多语言支持
- 数据自动更新

## 项目结构
```
WeatherApp/
├── backend/                     # C++后端服务
│   ├── include/               # 头文件目录
│   │   ├── weather_service.h  # 天气服务接口
│   │   ├── api_client.h       # API客户端
│   │   └── icon_renderer.h    # 图标渲染器
│   ├── src/                   # 源文件目录
│   │   ├── main.cpp          # 主程序入口
│   │   ├── weather_service.cpp
│   │   ├── api_client.cpp
│   │   └── icon_renderer.cpp
│   ├── CMakeLists.txt        # CMake构建文件
│   └── third_party/          # 第三方库
├── frontend/                  # C#前端界面
│   ├── MainWindow.xaml       # 主窗口界面
│   ├── MainWindow.xaml.cs    # 主窗口代码
│   ├── WeatherService.cs     # 天气服务接口
│   └── WeatherApp.csproj     # 项目文件
├── shared/                   # 共享资源
│   └── weather_data.h        # 共享数据结构
└── run.bat                  # 启动脚本
```
## 快速开始

### 环境要求

后端开发环境：
- CMake 3.15或更高版本
- C++17兼容编译器（GCC 7+、Clang 5+、MSVC 2019+）
- libcurl开发库

前端开发环境：
- .NET 6.0 SDK或更高版本
- Visual Studio 2022或Visual Studio Code
