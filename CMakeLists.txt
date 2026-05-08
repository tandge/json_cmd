cmake_minimum_required(VERSION 3.14)

# ======================== arch 参数处理 ========================
# 用法: cmake -Darch=lin64 ..  或  cmake -Darch=win64 ..
# 不指定则根据 CMAKE_SYSTEM_NAME 自动推断
set(arch "" CACHE STRING "Target platform: lin64 or win64. Leave empty for auto-detect.")

if(NOT arch STREQUAL "")
    # 校验合法性
    if(NOT arch STREQUAL "lin64" AND NOT arch STREQUAL "win64")
        message(FATAL_ERROR "Invalid arch='${arch}'. Must be 'lin64' or 'win64'.")
    endif()
    set(PLATFORM_TAG "${arch}")
    message(STATUS "arch explicitly set to: ${arch}")
else()
    # 未指定 arch，根据工具链自动推断
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(PLATFORM_TAG "win64")
    else()
        set(PLATFORM_TAG "lin64")
    endif()
    message(STATUS "arch not specified, auto-detected: ${PLATFORM_TAG}")
endif()

project(JsonDemo LANGUAGES CXX)

# 要求 C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ======================== Windows 交叉编译：静态链接 ========================
# 让 .exe 不依赖 libgcc_s_seh-1.dll / libstdc++-6.dll / libwinpthread-1.dll
# 这样 wine 可以直接运行，无需额外 DLL
if(PLATFORM_TAG STREQUAL "win64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libgcc -static-libstdc++")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
endif()

# ======================== 主程序：json_cmd ========================
add_executable(json_cmd main.cpp)
target_include_directories(json_cmd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# 设置输出文件名带平台标记，避免跨平台构建时互相覆盖
# 注意：不要在 OUTPUT_NAME 里加 .exe，CMake 会自动追加
set_target_properties(json_cmd PROPERTIES
    OUTPUT_NAME "json_cmd_${PLATFORM_TAG}"
)

# ======================== Google Test 配置 ========================
enable_testing()

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
# 防止在 MSVC 上覆盖编译器/链接器设置
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# 不安装 gtest
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ======================== 测试可执行文件 ========================
add_executable(test_json test_json.cpp)
target_include_directories(test_json PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_json GTest::gtest_main)
set_target_properties(test_json PROPERTIES
    OUTPUT_NAME "test_json_${PLATFORM_TAG}"
)

# ======================== CTest 注册 ========================
# Windows .exe 在 Linux 下需要通过 wine 运行
if(PLATFORM_TAG STREQUAL "win64")
    find_program(WINE_EXECUTABLE wine)
    if(WINE_EXECUTABLE)
        add_test(NAME test_json COMMAND ${WINE_EXECUTABLE} $<TARGET_FILE:test_json>)
    else()
        add_test(NAME test_json COMMAND $<TARGET_FILE:test_json>)
    endif()
else()
    add_test(NAME test_json COMMAND test_json)
endif()
