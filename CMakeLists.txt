# 设置 CMake 最低版本
cmake_minimum_required(VERSION 3.10)

# 项目名称和版本
project(WebServer VERSION 1.0)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 设置头文件目录（全局）
include_directories(${PROJECT_SOURCE_DIR}/include)

# 查找 src 目录下所有 .cpp 源文件
file(GLOB SOURCES "src/*.cpp")

# 添加可执行文件目标
add_executable(WebServer ${SOURCES})

# 链接 Boost.Asio 所需的库
target_link_libraries(WebServer PRIVATE -lpthread -lboost_system)