# json_cmd_tool

**Author:** 葛宁 \<tandge@gmail.com\>

## 简介

json_cmd_tool 是一个轻量级命令行 JSON 键值对遍历工具，支持 Linux / Windows 跨平台编译与运行。

将指定 JSON 文件中的所有叶节点以 `路径 = 值` 的形式打印输出，方便快速查看和检索 JSON 配置内容。核心功能：

- 递归遍历 JSON 对象和数组，输出所有叶子节点的完整路径与值
- 支持所有 JSON 数据类型：字符串、数字（整数/浮点）、布尔值、null
- 数组索引使用 `[n]` 语法，对象路径使用 `.` 分隔符，层级清晰
- 整数输出不带小数点，浮点数自动去尾零
- 解析错误输出行号与列号信息

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
| `-i` | 指定要解析的 JSON 文件路径（必填） |

### 退出码

| 退出码 | 含义 |
|--------|------|
| 0 | 成功 |
| 1 | 参数缺失或格式错误 |
| 2 | 无法读取输入文件 |
| 3 | JSON 解析失败 |

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
