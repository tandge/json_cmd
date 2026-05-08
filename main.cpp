/**
 * @file main.cpp
 * @brief 命令行 JSON 键值对遍历工具
 *
 * 把 JSON 文件里所有叶节点按 "路径 = 值" 打出来。
 * 用法: ./json_cmd -i <json_file>
 */

#include "json.hpp"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

// 退出码
static const int EXIT_USAGE   = 1;
static const int EXIT_FILE    = 2;
static const int EXIT_PARSE   = 3;

// 把数字格式化成可读字符串，整数不带小数点，浮点数用 %g 去尾零
static std::string fmtNum(double n) {
    char buf[64];
    if (std::isfinite(n) && n == static_cast<long long>(n)) {
        std::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(n));
    } else {
        std::snprintf(buf, sizeof(buf), "%g", n);
    }
    return buf;
}

// 叶节点值转字符串
static std::string fmtValue(const json::Value& v) {
    if (v.is_string())  return "\"" + v.as_string() + "\"";
    if (v.is_number())  return fmtNum(v.as_number());
    if (v.is_boolean()) return v.as_boolean() ? "true" : "false";
    if (v.is_null())    return std::string("null");
    return "";  // 不会走到这里
}

// 递归遍历，把所有叶节点以 "路径 = 值" 输出
static void traverse(const json::Value& v, const std::string& path,
                     std::ostream& out) {
    if (v.is_object()) {
        for (const auto& kv : v.as_object()) {
            std::string p = path.empty() ? kv.first : path + "." + kv.first;
            traverse(kv.second, p, out);
        }
    } else if (v.is_array()) {
        for (size_t i = 0; i < v.as_array().size(); ++i) {
            traverse(v.as_array()[i], path + "[" + std::to_string(i) + "]", out);
        }
    } else {
        out << path << " = " << fmtValue(v) << "\n";
    }
}

// 读取文件全部内容，失败返回空
static std::string readFile(const std::string& filepath) {
    std::ifstream f(filepath);
    if (!f.is_open()) return "";

    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

int main(int argc, char* argv[]) {
    // 解析 -i 参数
    std::string input_file;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-i" && i + 1 < argc) {
            input_file = argv[++i];
            break;
        }
    }

    if (input_file.empty()) {
        std::cerr << "Usage: " << argv[0] << " -i <json_file>\n";
        return EXIT_USAGE;
    }

    // 读文件
    std::string content = readFile(input_file);
    if (content.empty()) {
        std::cerr << "Error: cannot read file " << input_file << "\n";
        return EXIT_FILE;
    }

    // 解析 & 输出
    try {
        json::Value root = json::parse(content);
        traverse(root, "", std::cout);
    } catch (const json::ParseError& e) {
        std::cerr << "JSON parse error: " << e.what() << "\n";
        return EXIT_PARSE;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
