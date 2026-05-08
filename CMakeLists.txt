cmake_minimum_required(VERSION 3.14)
project(JsonDemo LANGUAGES CXX)

# 要求 C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加可执行文件
add_executable(json_cmd main.cpp)

# （可选）设置头文件搜索路径，让编译器找到 json.hpp
target_include_directories(json_cmd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ======================== Google Test 配置 ========================
enable_testing()

# 通过 FetchContent 自动下载 Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
# 防止在 MSVC 上覆盖编译器/链接器设置
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# 添加测试可执行文件
add_executable(test_json test_json.cpp)
target_include_directories(test_json PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_json GTest::gtest_main)

# 注册 CTest 测试（手动添加，避免 GTest 模块查找问题）
add_test(NAME test_json COMMAND test_json)
