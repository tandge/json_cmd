# json_cmd_tool

**Author:** 葛宁 \<tandge@gmail.com\>

一个轻量级命令行 JSON 键值对遍历工具。将指定 JSON 文件中的所有叶节点以 `路径 = 值` 的形式打印输出，方便快速查看和检索 JSON 配置内容。

## 功能特性

- 递归遍历 JSON 对象和数组，输出所有叶子节点的完整路径与值
- 支持所有 JSON 数据类型：字符串、数字、布尔值、null
- 数组索引使用 `[n]` 语法，符合 JSON Path 通用约定
- 对象路径使用 `.` 分隔符，层级清晰
- 友好的错误提示，包含解析错误的行号与列号信息

## 示例输出

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

运行程序后输出：

```
name = "app"
version = 2.1
debug = false
database.host = "localhost"
database.ports[0] = 8080
database.ports[1] = 8081
```

## 环境要求

- **编译器**：支持 C++17（GCC ≥ 7, Clang ≥ 5, MSVC ≥ 2017）
- **构建工具**：CMake ≥ 3.14
- **依赖**：无外部依赖，项目自带 `json.hpp` 单头文件 JSON 解析库

## 构建方式

### 使用构建脚本（推荐）

```bash
./build.sh
```

### 手动构建

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

构建完成后，可执行文件位于 `build/json_cmd`。

## 使用方法

```bash
./json_cmd -i <json_file>
```

### 参数说明

| 参数 | 说明 |
|------|------|
| `-i` | 指定要解析的 JSON 文件路径（必填） |

### 示例

```bash
./build/json_cmd -i config.json
```

## 项目结构

```
json_cmd_tool/
├── CMakeLists.txt   # CMake 构建配置
├── build.sh         # 一键构建脚本
├── config.json      # 示例 JSON 配置文件
├── json.hpp         # 单头文件 JSON 解析库
├── main.cpp         # 主程序源码
└── README.md        # 项目说明文档
```

## 许可证

MIT License
