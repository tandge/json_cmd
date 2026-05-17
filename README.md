# json_cmd_tool

**Author:** 葛宁 (Ning Ge) \<tandge@gmail.com\>

<details>
<summary>中文 / Click for Chinese</summary>

## 简介

json_cmd_tool 是一个轻量级命令行 JSON 处理工具，支持 Linux / Windows 跨平台编译与运行。

🌐 **在线版**：[https://tandge.github.io/json_cmd/](https://tandge.github.io/json_cmd/) — Linux、macOS、Windows、iOS、Android 均可通过浏览器直接访问使用，无需安装。

将指定 JSON 文件中的所有叶节点以 `路径 = 值` 的形式打印输出，方便快速查看和检索 JSON 配置内容。核心功能：

- 递归遍历 JSON 对象和数组，输出所有叶子节点的完整路径与值
- 支持所有 JSON 数据类型：字符串、数字（整数/浮点）、布尔值、null
- 数组索引使用 `[n]` 语法，对象路径使用 `.` 分隔符，层级清晰
- 整数输出不带小数点，浮点数自动去尾零
- 解析错误输出行号与列号信息
- 校验模式：仅检查 JSON 合法性
- 压缩模式：输出单行紧凑 JSON
- 格式化模式：按缩进重新格式化输出
- 原样输出：直接打印文件原始内容
- key 排序：输出时对对象 key 按字典序排序
- key 过滤：支持通配符 `*` 和 `?` 按模式筛选 key

### 示例

给定以下 `config.json`：

```json
{
    "name": "app",
    "version": 2.1,
    "debug": false,
    "database": {
        "host": "localhost",
        "ports": [8080, 8081]
    }
}
```

运行后输出：

```
name = "app"
version = 2.1
debug = false
database.host = "localhost"
database.ports[0] = 8080
database.ports[1] = 8081
```

更多用法示例：

```bash
# 校验 JSON 是否合法
./json_cmd_lin64 -i config.json -v

# 压缩为单行
./json_cmd_lin64 -i config.json -c

# 格式化输出（默认2空格缩进）
./json_cmd_lin64 -i config.json -F

# 格式化输出（4空格缩进）
./json_cmd_lin64 -i config.json -F -n 4

# 原样输出文件内容
./json_cmd_lin64 -i config.json -p

# 遍历输出（key 按字典序排序）
./json_cmd_lin64 -i config.json -s

# 仅输出 key 匹配 "d*" 的条目
./json_cmd_lin64 -i config.json -k "d*"

# 仅输出 key 匹配 "d*" 的条目，并格式化
./json_cmd_lin64 -i config.json -k "d*" -F

# 压缩输出，key 排序，仅匹配 "n?me"
./json_cmd_lin64 -i config.json -s -k "n?me" -c
```

## 运行环境

### Linux

- x86_64 架构，glibc 2.17 及以上
- 无额外运行时依赖

### Windows

- Windows 7 及以上（64-bit）
- Windows 版本采用静态链接编译，不依赖 `libgcc_s_seh-1.dll`、`libstdc++-6.dll`、`libwinpthread-1.dll` 等运行时库，可独立运行

## 编译构建

### 编译环境要求

| 工具 | 最低版本 | 用途 |
|------|----------|------|
| CMake | ≥ 3.14 | 构建系统 |
| GCC / Clang | 支持 C++17 | Linux 原生编译 |
| MinGW-w64 | `x86_64-w64-mingw32-g++-posix` | Windows 交叉编译 |
| Wine | 任意版本（可选） | 在 Linux 上运行 Windows 单元测试 |

项目自带 `json.hpp` 单头文件 JSON 解析库，无其他外部依赖。单元测试框架 Google Test 通过 CMake FetchContent 自动下载。

### 使用构建脚本（推荐）

通过参数指定构建平台：

```bash
./build.sh            # 同时构建 lin64 + win64
./build.sh lin64      # 仅构建 Linux 64bit
./build.sh win64      # 仅构建 Windows 64bit
```

脚本执行流程：清理旧构建 → 逐平台 cmake 配置 + 编译 + 单元测试 → 汇总输出。

### Linux 编译

使用构建脚本：

```bash
./build.sh lin64
```

手动编译：

```bash
mkdir build/lin64 && cd build/lin64
cmake ../.. -Darch=lin64 -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
ctest --output-on-failure
```

构建产物：

| 文件 | 说明 |
|------|------|
| `build/lin64/json_cmd_lin64` | 主程序 |
| `build/lin64/test_json_lin64` | 单元测试 |

### Windows 编译

#### 交叉编译（Linux → Windows，推荐）

在 Linux 主机上使用 MinGW-w64 交叉编译，生成 Windows 64-bit 可执行文件。

使用构建脚本：

```bash
./build.sh win64
```

手动编译：

```bash
mkdir build/win64 && cd build/win64
cmake ../.. -Darch=win64 \
    -DCMAKE_TOOLCHAIN_FILE=../toolchain-mingw64.cmake \
    -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
ctest --output-on-failure   # 需要 wine
```

构建产物：

| 文件 | 说明 |
|------|------|
| `build/win64/json_cmd_win64.exe` | 主程序 |
| `build/win64/test_json_win64.exe` | 单元测试 |

#### MinGW 原生编译（Windows 上）

在 Windows 环境下使用 MinGW-w64 直接编译：

```bat
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -Darch=win64 -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
```

> **注意**：此方式未在 CI 中测试，如遇问题建议使用 Linux 交叉编译。

## 使用方式

### Linux

```bash
./json_cmd_lin64 -i <json_file>
```

示例：

```bash
./build/lin64/json_cmd_lin64 -i config.json
```

### Windows

```cmd
json_cmd_win64.exe -i <json_file>
```

在 Linux 上通过 wine 运行：

```bash
wine build/win64/json_cmd_win64.exe -i config.json
```

### 参数说明

| 参数 | 说明 |
|------|------|
| `-i` / `-f` | 指定要解析的 JSON 文件路径（必填，两者等价） |
| `-v` | 校验模式，仅检查 JSON 合法性 |
| `-c` | 压缩模式，输出单行紧凑 JSON |
| `-F` | 格式化模式，按缩进重新输出 |
| `-n N` | 每级缩进空格数（默认 2，需配合 `-F`） |
| `-p` | 原样输出，直接打印文件原始内容 |
| `-s` | 输出时对对象 key 按字典序排序 |
| `-k pattern` | key 过滤模式，支持通配符 `*`（任意多字符）和 `?`（单个字符） |

### 退出码

| 退出码 | 含义 |
|--------|------|
| 0 | 成功 |
| 1 | 参数缺失或格式错误 |
| 2 | 无法读取输入文件 |
| 3 | JSON 解析失败 |
| 4 | 校验失败 |
| 5 | 压缩失败 |
| 6 | 格式化失败 |

## 下载发行版

提供 Linux / Windows 预编译二进制包，见 [Releases](https://github.com/tandge/json_cmd_tool/releases) 页面。

| 平台 | 文件 |
|------|------|
| Linux 64-bit | `json_cmd_lin64` |
| Windows 64-bit | `json_cmd_win64.exe` |

下载后赋予执行权限即可直接运行，无需安装。

## 常见问题

### Linux 常见问题

**Q: 运行时提示 `Permission denied`**

A: 下载的二进制文件需要添加执行权限：

```bash
chmod +x json_cmd_lin64
```

**Q: 运行时提示 `/lib64/libc.so.6: version 'GLIBC_2.xx' not found`**

A: 二进制在较新的 Linux 上编译，glibc 版本高于当前系统。请升级系统或在目标系统上自行编译。

**Q: 编译时找不到 C++17 编译器**

A: 确认 GCC 版本 ≥ 7：

```bash
g++ --version
```

Ubuntu 18.04+ / CentOS 8+ 自带的 GCC 均满足要求。

### Windows 常见问题

**Q: 双击 .exe 闪退**

A: 本程序是命令行工具，需要通过 `cmd` 或 PowerShell 运行。请打开终端后执行：

```cmd
json_cmd_win64.exe -i <json_file>
```

**Q: 运行时提示缺少 DLL**

A: 本项目 Windows 版本采用静态链接，正常情况下不会出现此问题。如果从源码自行编译且未启用静态链接，可能缺少 MinGW 运行时 DLL。建议使用项目提供的 `toolchain-mingw64.cmake` 重新编译，或直接下载预编译版本。

**Q: wine 运行 .exe 报错 `Library libwinpthread-1.dll not found`**

A: 交叉编译时未启用静态链接。确认 CMake 配置中包含 `-Darch=win64` 参数，CMakeLists.txt 会自动添加 `-static` 相关标志。

## 项目结构

```
json_cmd_tool/
├── CMakeLists.txt             # CMake 构建配置（支持双平台，-Darch=lin64/win64）
├── toolchain-mingw64.cmake    # MinGW-w64 交叉编译工具链文件
├── build.sh                   # 一键构建脚本
├── config.json                # 示例 JSON 配置文件
├── json.hpp                   # 单头文件 JSON 解析库
├── main.cpp                   # 主程序源码
├── test_json.cpp              # 单元测试源码
├── CHANGELOG.md               # 变更日志
└── README.md                  # 项目说明文档
```

## 许可证

MIT License

</details>

<details open>
<summary>English / 点击切换中文</summary>

## Introduction

json_cmd_tool is a lightweight command-line JSON processing tool that supports cross-platform compilation and execution on Linux / Windows.

🌐 **Online Version**: [https://tandge.github.io/json_cmd/](https://tandge.github.io/json_cmd/) — Accessible directly via browser on Linux, macOS, Windows, iOS, and Android. No installation required.

It prints all leaf nodes of a specified JSON file in the format `path = value`, making it easy to quickly view and search JSON configuration content. Core features:

- Recursively traverse JSON objects and arrays, outputting the full path and value of all leaf nodes
- Support all JSON data types: strings, numbers (integer/float), booleans, null
- Array indices use `[n]` syntax, object paths use `.` separator for clear hierarchy
- Integers are output without decimal points, floating-point numbers automatically have trailing zeros removed
- Parse errors output line and column number information
- Validation mode: only check JSON validity
- Minify mode: output single-line compact JSON
- Format mode: reformat output with indentation
- Raw output: print the file's original content directly
- Key sorting: sort object keys in lexicographic order during output
- Key filtering: support wildcards `*` and `?` for pattern-based key filtering

### Example

Given the following `config.json`:

```json
{
    "name": "app",
    "version": 2.1,
    "debug": false,
    "database": {
        "host": "localhost",
        "ports": [8080, 8081]
    }
}
```

Output after running:

```
name = "app"
version = 2.1
debug = false
database.host = "localhost"
database.ports[0] = 8080
database.ports[1] = 8081
```

More usage examples:

```bash
# Validate JSON
./json_cmd_lin64 -i config.json -v

# Minify to single line
./json_cmd_lin64 -i config.json -c

# Format output (default 2-space indentation)
./json_cmd_lin64 -i config.json -F

# Format output (4-space indentation)
./json_cmd_lin64 -i config.json -F -n 4

# Raw output of file content
./json_cmd_lin64 -i config.json -p

# Traverse output (keys sorted lexicographically)
./json_cmd_lin64 -i config.json -s

# Only output entries where key matches "d*"
./json_cmd_lin64 -i config.json -k "d*"

# Only output entries where key matches "d*", with formatting
./json_cmd_lin64 -i config.json -k "d*" -F

# Minified output, keys sorted, only match "n?me"
./json_cmd_lin64 -i config.json -s -k "n?me" -c
```

## Runtime Environment

### Linux

- x86_64 architecture, glibc 2.17 and above
- No additional runtime dependencies

### Windows

- Windows 7 and above (64-bit)
- The Windows version is compiled with static linking, and does not depend on runtime libraries such as `libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`, etc. It can run standalone

## Build

### Build Environment Requirements

| Tool | Minimum Version | Purpose |
|------|-----------------|---------|
| CMake | ≥ 3.14 | Build system |
| GCC / Clang | C++17 support | Linux native compilation |
| MinGW-w64 | `x86_64-w64-mingw32-g++-posix` | Windows cross-compilation |
| Wine | Any version (optional) | Run Windows unit tests on Linux |

The project includes the `json.hpp` single-header JSON parsing library with no other external dependencies. The unit testing framework Google Test is automatically downloaded via CMake FetchContent.

### Using the Build Script (Recommended)

Specify the build platform via arguments:

```bash
./build.sh            # Build both lin64 + win64
./build.sh lin64      # Build Linux 64-bit only
./build.sh win64      # Build Windows 64-bit only
```

Script execution flow: clean old builds → cmake configure + compile + unit test per platform → summary output.

### Linux Build

Using the build script:

```bash
./build.sh lin64
```

Manual build:

```bash
mkdir build/lin64 && cd build/lin64
cmake ../.. -Darch=lin64 -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
ctest --output-on-failure
```

Build artifacts:

| File | Description |
|------|-------------|
| `build/lin64/json_cmd_lin64` | Main program |
| `build/lin64/test_json_lin64` | Unit tests |

### Windows Build

#### Cross-compilation (Linux → Windows, Recommended)

Use MinGW-w64 on a Linux host to cross-compile and generate Windows 64-bit executables.

Using the build script:

```bash
./build.sh win64
```

Manual build:

```bash
mkdir build/win64 && cd build/win64
cmake ../.. -Darch=win64 \
    -DCMAKE_TOOLCHAIN_FILE=../toolchain-mingw64.cmake \
    -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
ctest --output-on-failure   # Requires wine
```

Build artifacts:

| File | Description |
|------|-------------|
| `build/win64/json_cmd_win64.exe` | Main program |
| `build/win64/test_json_win64.exe` | Unit tests |

#### MinGW Native Build (On Windows)

Compile directly on Windows using MinGW-w64:

```bat
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -Darch=win64 -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
```

> **Note**: This method is not tested in CI. If you encounter issues, it is recommended to use Linux cross-compilation.

## Usage

### Linux

```bash
./json_cmd_lin64 -i <json_file>
```

Example:

```bash
./build/lin64/json_cmd_lin64 -i config.json
```

### Windows

```cmd
json_cmd_win64.exe -i <json_file>
```

Run via wine on Linux:

```bash
wine build/win64/json_cmd_win64.exe -i config.json
```

### Parameter Reference

| Parameter | Description |
|-----------|-------------|
| `-i` / `-f` | Specify the JSON file path to parse (required, both are equivalent) |
| `-v` | Validation mode, only check JSON validity |
| `-c` | Minify mode, output single-line compact JSON |
| `-F` | Format mode, reformat output with indentation |
| `-n N` | Number of spaces per indentation level (default 2, requires `-F`) |
| `-p` | Raw output, print the file's original content directly |
| `-s` | Sort object keys in lexicographic order during output |
| `-k pattern` | Key filter mode, supports wildcards `*` (any number of characters) and `?` (single character) |

### Exit Codes

| Exit Code | Meaning |
|-----------|---------|
| 0 | Success |
| 1 | Missing or malformed arguments |
| 2 | Cannot read input file |
| 3 | JSON parse failure |
| 4 | Validation failure |
| 5 | Minification failure |
| 6 | Formatting failure |

## Download Releases

Pre-compiled binary packages for Linux / Windows are available on the [Releases](https://github.com/tandge/json_cmd_tool/releases) page.

| Platform | File |
|----------|------|
| Linux 64-bit | `json_cmd_lin64` |
| Windows 64-bit | `json_cmd_win64.exe` |

After downloading, grant execute permission and run directly — no installation required.

## FAQ

### Linux FAQ

**Q: Getting `Permission denied` when running**

A: The downloaded binary needs execute permission:

```bash
chmod +x json_cmd_lin64
```

**Q: Getting `/lib64/libc.so.6: version 'GLIBC_2.xx' not found` when running**

A: The binary was compiled on a newer Linux with a glibc version higher than your system. Please upgrade your system or compile from source on the target system.

**Q: Cannot find a C++17 compiler when building**

A: Confirm GCC version ≥ 7:

```bash
g++ --version
```

GCC shipped with Ubuntu 18.04+ / CentOS 8+ meets this requirement.

### Windows FAQ

**Q: Double-clicking .exe flashes and closes immediately**

A: This is a command-line tool that needs to be run via `cmd` or PowerShell. Open a terminal and execute:

```cmd
json_cmd_win64.exe -i <json_file>
```

**Q: Getting missing DLL errors when running**

A: The Windows version of this project uses static linking, so this should not normally happen. If you compiled from source without enabling static linking, you may be missing MinGW runtime DLLs. It is recommended to recompile using the project's provided `toolchain-mingw64.cmake`, or download the pre-compiled release directly.

**Q: Running .exe with wine gives error `Library libwinpthread-1.dll not found`**

A: Static linking was not enabled during cross-compilation. Confirm that your CMake configuration includes the `-Darch=win64` parameter — CMakeLists.txt will automatically add the `-static` related flags.

## Project Structure

```
json_cmd_tool/
├── CMakeLists.txt             # CMake build configuration (dual-platform, -Darch=lin64/win64)
├── toolchain-mingw64.cmake    # MinGW-w64 cross-compilation toolchain file
├── build.sh                   # One-click build script
├── config.json                # Sample JSON configuration file
├── json.hpp                   # Single-header JSON parsing library
├── main.cpp                   # Main program source code
├── test_json.cpp              # Unit test source code
├── CHANGELOG.md               # Changelog
└── README.md                  # Project documentation
```

## License

MIT License

</details>
