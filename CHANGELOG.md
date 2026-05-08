# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
