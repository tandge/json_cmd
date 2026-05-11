# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.3.0] - 2026-05-11

### Added

#### 校验模式 (`-v`)

- 新增 `-v` 参数，仅对 JSON 文件做合法性检查，不遍历输出
- 校验通过输出 `JSON validation passed.`
- 校验失败输出行号、列号、错误类型

#### 压缩模式 (`-c`)

- 新增 `-c` 参数，将 JSON 文件压缩为单行紧凑格式
- 去除所有多余空白（空格、换行、缩进），保留语义不变

#### 格式化模式 (`-F` / `-n`)

- 新增 `-F` 参数，将 JSON 压缩后重新按缩进格式化输出
- 新增 `-n N` 参数，指定每级缩进的空格数（默认 2，需配合 `-F`）

#### 原样输出 (`-p`)

- 新增 `-p` 参数，将 JSON 文件内容原样输出到控制台，不解析不格式化

#### key 排序 (`-s`)

- 新增 `-s` 参数，输出时对 JSON 对象的 key 按字典序排序（参考 jq `--sort-keys`）
- 可与 `-c` / `-F` / 默认遍历模式配合使用

#### key 过滤 (`-k`)

- 新增 `-k pattern` 参数，按通配符模式筛选 key，仅输出匹配的条目
- `*` 匹配任意多字符，`?` 匹配任意单个字符
- 匹配的 key 保留完整子树
- 可与 `-c` / `-F` / `-s` / 默认遍历模式配合使用

#### 参数兼容

- `-i` 与 `-f` 均可指定输入 JSON 文件，行为完全等价（兼容 v0.1.0 的 `-i` 用法）

### Changed

- 退出码扩展：新增 4（校验失败）、5（压缩失败）、6（格式化失败）

[0.3.0]: https://github.com/tandge/json_cmd_tool/releases/tag/v0.3.0

## [0.2.0] - 2026-05-08

### Added

#### 跨平台构建支持

- 新增 MinGW-w64 交叉编译工具链文件 `toolchain-mingw64.cmake`，支持在 Linux 上编译 Windows 64bit 二进制文件
- CMakeLists.txt 支持通过 `-Darch=lin64` 或 `-Darch=win64` 参数指定编译目标平台
- 不指定 `arch` 时自动根据工具链推断平台（Linux 原生编译为 lin64，MinGW 交叉编译为 win64）
- `arch` 参数非法值时给出明确错误提示
- 构建产物带平台标记（`json_cmd_lin64` / `json_cmd_win64.exe`），避免跨平台构建时互相覆盖
- Windows 版本采用静态链接（`-static -static-libgcc -static-libstdc++`），无需额外运行时 DLL

#### 跨平台单元测试

- Windows 单元测试通过 wine 在 Linux 宿主上运行，ctest 自动检测并使用 wine
- `build.sh` 支持三种调用方式：`./build.sh`（双平台）、`./build.sh lin64`（仅 Linux）、`./build.sh win64`（仅 Windows）

### Changed

- `build.sh` 从单一 Linux 构建扩展为可按平台选择的双平台构建 + 测试流程
- 构建目录结构改为 `build/lin64/` 和 `build/win64/` 分平台存放
- CMakeLists.txt 中 `enable_testing()` 移至 FetchContent 之前，确保 ctest 可用
- README.md 按照"简介 → 运行环境 → 编译构建 → 使用方式 → 下载发行版 → 常见问题"大纲重新组织，补充运行环境、退出码、FAQ 等细节

[0.2.0]: https://github.com/tandge/json_cmd_tool/releases/tag/v0.2.0

## [0.1.0] - 2026-04-30

### Added

#### json_cmd 命令行工具

- 递归遍历 JSON 文件中所有叶节点，以 `路径 = 值` 格式输出
- 支持 `-i` 参数指定输入 JSON 文件路径
- 对象路径使用 `.` 分隔符，数组索引使用 `[n]` 语法
- 支持全部 JSON 数据类型：字符串、数字（整数/浮点）、布尔值、null
- 整数输出不带小数点，浮点数自动去尾零
- 解析错误输出行号与列号信息
- 三种退出码区分错误类型：参数缺失(1)、文件不可读(2)、解析失败(3)

#### json.hpp 单头文件 JSON 解析库

- 完全符合 RFC 8259（JSON 标准）的递归下降解析器
- 基于 `string_view` 的零拷贝解析，仅在需要时拷贝字符串
- 详尽的错误报告：`ParseError` 异常携带行号、列号及具体问题描述
- DOM 接口：`Value` 类支持 Null / Boolean / Number / String / Array / Object 六种类型
- 对象异构查找：`std::map<std::string, Value, std::less<>>` 支持 `string_view` 键查找
- Unicode 支持：正确处理 `\uXXXX` 转义及 UTF-16 代理对
- 序列化功能：`dump()` 方法将 Value 树序列化为紧凑 JSON 字符串
- 便捷工厂函数：`Array()` / `Object()` 快速构造数组和对象
- 初始化列表构造：支持 `Value{1.0, 2.0}` 和 `Value{{"k", v}}` 语法
- 无第三方依赖，仅使用 C++17 标准库

#### 构建与测试

- CMake 构建系统（要求 CMake ≥ 3.14，C++17 编译器）
- 一键构建脚本 `build.sh`
- Google Test 单元测试框架（通过 FetchContent 自动下载 gtest v1.14.0）
- 119 个测试用例覆盖：值构造与类型查询、类型转换与访问、数组/对象下标操作、
  比较运算、序列化、字面量/数字/字符串/数组/对象解析、Unicode 转义与代理对、
  错误报告、初始化列表与工厂函数、往返一致性、边界情况

### Changed

- 可执行文件名从 `demo` 更改为 `json_cmd`

### Fixed

（首个发布版本，无历史修复记录）

[0.1.0]: https://github.com/tandge/json_cmd_tool/releases/tag/v0.1.0
