/**
 * @file main.cpp
 * @brief 命令行 JSON 键值对遍历与校验工具
 *
 * 功能：
 *   -f  指定 JSON 文件，遍历所有叶节点按 "路径 = 值" 输出
 *   -v  校验模式，仅对 -f 指定的 JSON 文件做合法性检查
 *       校验失败时输出：行号、列号、错误类型
 *   -c  压缩模式，将 JSON 文件压缩为单行紧凑格式
 *       去除所有多余空白（空格、换行、缩进），保留语义不变
 *   -F  格式化模式，将 JSON 压缩后重新按缩进格式化输出
 *   -n  指定每级缩进的空格数（默认 2，仅与 -F 配合使用）
 *
 * 用法:
 *   ./json_cmd -f <json_file>           遍历输出
 *   ./json_cmd -f <json_file> -v        仅校验
 *   ./json_cmd -f <json_file> -c        压缩为单行
 *   ./json_cmd -f <json_file> -F        格式化输出（默认2空格缩进）
 *   ./json_cmd -f <json_file> -F -n 4   格式化输出（4空格缩进）
 */

#include "json.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

// 退出码
static const int EXIT_USAGE      = 1;
static const int EXIT_FILE       = 2;
static const int EXIT_PARSE      = 3;
static const int EXIT_VALIDATE   = 4;
static const int EXIT_COMPACT    = 5;
static const int EXIT_FORMAT     = 6;

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

/**
 * @brief 递归格式化输出 JSON，每级按指定空格数缩进
 *
 * @param v       JSON 值
 * @param indent  每级缩进空格数
 * @param level   当前嵌套层级（根节点为 0）
 * @param out     输出流
 */
static void dumpPretty(const json::Value& v, int indent, int level,
                       std::ostream& out) {
    std::string pad(level * indent, ' ');
    std::string pad1((level + 1) * indent, ' ');

    if (v.is_null()) {
        out << "null";
    } else if (v.is_boolean()) {
        out << (v.as_boolean() ? "true" : "false");
    } else if (v.is_number()) {
        out << fmtNum(v.as_number());
    } else if (v.is_string()) {
        // 复用 dump() 保证转义正确
        out << json::Value(v.as_string()).dump();
    } else if (v.is_array()) {
        const auto& arr = v.as_array();
        if (arr.empty()) { out << "[]"; return; }
        out << "[\n";
        for (size_t i = 0; i < arr.size(); ++i) {
            out << pad1;
            dumpPretty(arr[i], indent, level + 1, out);
            if (i + 1 < arr.size()) out << ",";
            out << "\n";
        }
        out << pad << "]";
    } else if (v.is_object()) {
        const auto& obj = v.as_object();
        if (obj.empty()) { out << "{}"; return; }
        out << "{\n";
        size_t idx = 0;
        for (const auto& [key, val] : obj) {
            out << pad1 << json::Value(key).dump() << ": ";
            dumpPretty(val, indent, level + 1, out);
            if (++idx < obj.size()) out << ",";
            out << "\n";
        }
        out << pad << "}";
    }
}

/**
 * @brief 将 JSON 文本压缩为单行紧凑格式
 *
 * 解析后重新序列化，自动去除所有多余空白。
 *
 * @param content  JSON 文本内容
 * @return 0 成功，非 0 失败
 */
static int compact(const std::string& content) {
    try {
        json::Value root = json::parse(content);
        std::cout << root.dump() << "\n";
        return 0;
    } catch (const json::ParseError& e) {
        std::cerr << "JSON compact failed:\n"
                  << "  line: " << e.line() << "\n"
                  << "  col:  " << e.column() << "\n"
                  << "  error: " << e.what() << "\n";
        return EXIT_COMPACT;
    } catch (const std::exception& e) {
        std::cerr << "JSON compact failed:\n"
                  << "  error: " << e.what() << "\n";
        return EXIT_COMPACT;
    }
}

/**
 * @brief 格式化 JSON 文本，按指定缩进重新输出
 *
 * @param content  JSON 文本内容
 * @param indent   每级缩进空格数
 * @return 0 成功，非 0 失败
 */
static int formatJson(const std::string& content, int indent) {
    try {
        json::Value root = json::parse(content);
        dumpPretty(root, indent, 0, std::cout);
        std::cout << "\n";
        return 0;
    } catch (const json::ParseError& e) {
        std::cerr << "JSON format failed:\n"
                  << "  line: " << e.line() << "\n"
                  << "  col:  " << e.column() << "\n"
                  << "  error: " << e.what() << "\n";
        return EXIT_FORMAT;
    } catch (const std::exception& e) {
        std::cerr << "JSON format failed:\n"
                  << "  error: " << e.what() << "\n";
        return EXIT_FORMAT;
    }
}

/**
 * @brief 校验 JSON 文本，输出详细的错误信息
 *
 * @param content  JSON 文本内容
 * @return 0 校验通过，非 0 校验失败
 */
static int validate(const std::string& content) {
    try {
        json::parse(content);
        std::cout << "JSON validation passed.\n";
        return 0;
    } catch (const json::ParseError& e) {
        std::cerr << "JSON validation failed:\n"
                  << "  line: " << e.line() << "\n"
                  << "  col:  " << e.column() << "\n"
                  << "  error: " << e.what() << "\n";
        return EXIT_VALIDATE;
    } catch (const std::exception& e) {
        std::cerr << "JSON validation failed:\n"
                  << "  error: " << e.what() << "\n";
        return EXIT_VALIDATE;
    }
}

int main(int argc, char* argv[]) {
    // 解析命令行参数
    std::string input_file;
    bool flag_validate = false;
    bool flag_compact  = false;
    bool flag_format   = false;
    int  indent_spaces = 2;
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "-f" && i + 1 < argc) {
            input_file = argv[++i];
        } else if (arg == "-v") {
            flag_validate = true;
        } else if (arg == "-c") {
            flag_compact = true;
        } else if (arg == "-F") {
            flag_format = true;
        } else if (arg == "-n" && i + 1 < argc) {
            indent_spaces = std::atoi(argv[++i]);
            if (indent_spaces <= 0) indent_spaces = 2;
        }
    }

    if (input_file.empty()) {
        std::cerr << "Usage: " << argv[0] << " -f <json_file> [-v] [-c] [-F [-n N]]\n"
                  << "  -f  input JSON file\n"
                  << "  -v  validate only (no traversal)\n"
                  << "  -c  compact to single line\n"
                  << "  -F  pretty-print with indentation\n"
                  << "  -n  spaces per indent level (default 2, use with -F)\n";
        return EXIT_USAGE;
    }

    // 读文件
    std::string content = readFile(input_file);
    if (content.empty()) {
        std::cerr << "Error: cannot read file " << input_file << "\n";
        return EXIT_FILE;
    }

    // 校验模式：仅检查合法性，不遍历输出
    if (flag_validate) {
        return validate(content);
    }

    // 压缩模式：输出单行紧凑 JSON
    if (flag_compact) {
        return compact(content);
    }

    // 格式化模式：压缩后重新按缩进输出
    if (flag_format) {
        return formatJson(content, indent_spaces);
    }

    // 默认模式：解析 & 遍历输出
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
