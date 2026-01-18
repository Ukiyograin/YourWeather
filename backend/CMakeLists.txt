cmake_minimum_required(VERSION 3.15)
project(WeatherServiceBackend)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找依赖
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/../shared)

# 设置编译器选项
if(MSVC)
    add_compile_options(/W4 /EHsc)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# 添加可执行文件
add_executable(weather_service_backend
    src/main.cpp
    src/weather_service.cpp
    src/api_client.cpp
    src/rpc_server.cpp
)

# 链接库
target_link_libraries(weather_service_backend
    ${CURL_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)

# 下载并包含 nlohmann/json
include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)
target_link_libraries(weather_service_backend nlohmann_json)

# 下载并包含 gRPC (如果需要)
if(BUILD_GRPC)
    include(FetchContent)
    FetchContent_Declare(
        grpc
        GIT_REPOSITORY https://github.com/grpc/grpc.git
        GIT_TAG v1.54.0
    )
    FetchContent_MakeAvailable(grpc)
    target_link_libraries(weather_service_backend grpc++)
endif()

# 安装目标
install(TARGETS weather_service_backend
    RUNTIME DESTINATION bin
    CONFIGURATIONS Release
)