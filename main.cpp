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
 *   -p  原样输出模式，将 JSON 文件内容直接输出到控制台
 *   -s  输出时对对象 key 按字典序排序（参考 jq --sort-seys）
 *       可与 -c / -F / 默认遍历模式配合使用
 *   -k  指定 key 过滤模式，支持通配符 * 和 ?
 *       * 匹配任意多字符，? 匹配任意单个字符
 *       仅输出 key 名匹配的条目，可与 -c / -F / -s / 默认遍历配合
 *
 * 用法:
 *   ./json_cmd -f <json_file>             遍历输出
 *   ./json_cmd -f <json_file> -v          仅校验
 *   ./json_cmd -f <json_file> -c          压缩为单行
 *   ./json_cmd -f <json_file> -F          格式化输出（默认2空格缩进）
 *   ./json_cmd -f <json_file> -F -n 4     格式化输出（4空格缩进）
 *   ./json_cmd -f <json_file> -p          原样输出
 *   ./json_cmd -f <json_file> -s          遍历输出（key 按字典序排序）
 *   ./json_cmd -f <json_file> -k "name*"  遍历输出（仅 key 匹配 name* 的条目）
 */

#include "json.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
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

/**
 * @brief 通配符匹配，支持 * 和 ?
 *
 * @param pattern  通配符模式，* 匹配任意多字符，? 匹配单个字符
 * @param text     待匹配的字符串
 * @return true 匹配，false 不匹配
 */
static bool wildcardMatch(const std::string& pattern, const std::string& text) {
    size_t pi = 0, ti = 0;
    size_t star_pos = std::string::npos;
    size_t match_pos = 0;

    while (ti < text.size()) {
        if (pi < pattern.size() && (pattern[pi] == text[ti] || pattern[pi] == '?')) {
            ++pi; ++ti;
        } else if (pi < pattern.size() && pattern[pi] == '*') {
            star_pos = pi++;
            match_pos = ti;
        } else if (star_pos != std::string::npos) {
            pi = star_pos + 1;
            ti = ++match_pos;
        } else {
            return false;
        }
    }
    // 跳过尾部多余的 *
    while (pi < pattern.size() && pattern[pi] == '*') ++pi;
    return pi == pattern.size();
}

/**
 * @brief 递归过滤 JSON 树，只保留 key 匹配通配符模式的条目
 *
 * 在每个对象层级上，移除 key 不匹配的条目；
 * 匹配的 key 保留其完整子树，不再对子树继续过滤。
 *
 * @param v          JSON 值（原地修改）
 * @param key_filter 通配符模式，空串表示不过滤
 */
static void filterByKey(json::Value& v, const std::string& key_filter) {
    if (key_filter.empty()) return;
    if (v.is_object()) {
        auto& obj = v.as_object();
        for (auto it = obj.begin(); it != obj.end(); ) {
            if (wildcardMatch(key_filter, it->first)) {
                ++it;  // 匹配则保留完整子树
            } else {
                it = obj.erase(it);
            }
        }
    } else if (v.is_array()) {
        for (auto& elem : v.as_array()) {
            filterByKey(elem, key_filter);
        }
    }
}

// 叶节点值转字符串
static std::string fmtValue(const json::Value& v) {
    if (v.is_string())  return "\"" + v.as_string() + "\"";
    if (v.is_number())  return fmtNum(v.as_number());
    if (v.is_boolean()) return v.as_boolean() ? "true" : "false";
    if (v.is_null())    return std::string("null");
    return "";  // 不会走到这里
}

/**
 * @brief 递归地确保 JSON 树中所有对象的 key 按字典序排列
 *
 * 当前 Object 使用 std::map（天然有序），此函数为 no-op；
 * 若将来 Object 换成无序容器，此函数可保证 -s 语义生效。
 *
 * @param v  JSON 值（原地修改）
 */
static void sortObjectKeys(json::Value& v) {
    if (v.is_object()) {
        // std::map 天然有序，无需重排；递归处理子值
        for (auto& kv : v.as_object()) {
            sortObjectKeys(kv.second);
        }
    } else if (v.is_array()) {
        for (auto& elem : v.as_array()) {
            sortObjectKeys(elem);
        }
    }
}

/**
 * @brief 收集对象的所有 key，按字典序排序后返回
 *
 * 当 sort_key 为 true 时对 key 显式排序；
 * 为 false 时保留 map 原始迭代顺序（std::map 天然有序，两者效果相同）。
 *
 * @param obj       JSON 对象
 * @param sort_key  是否按字典序排序 key
 * @return 按 key 排序后的键值对向量
 */
static std::vector<std::pair<std::string, std::reference_wrapper<const json::Value>>>
sortedKeys(const json::Value::Object& obj, bool sort_key) {
    std::vector<std::pair<std::string, std::reference_wrapper<const json::Value>>> entries;
    entries.reserve(obj.size());
    for (const auto& kv : obj) {
        entries.emplace_back(kv.first, std::cref(kv.second));
    }
    if (sort_key) {
        std::sort(entries.begin(), entries.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });
    }
    return entries;
}

// 递归遍历，把所有叶节点以 "路径 = 值" 输出
static void traverse(const json::Value& v, const std::string& path,
                     std::ostream& out, bool sort_key) {
    if (v.is_object()) {
        auto entries = sortedKeys(v.as_object(), sort_key);
        for (const auto& kv : entries) {
            std::string p = path.empty() ? kv.first : path + "." + kv.first;
            traverse(kv.second.get(), p, out, sort_key);
        }
    } else if (v.is_array()) {
        for (size_t i = 0; i < v.as_array().size(); ++i) {
            traverse(v.as_array()[i], path + "[" + std::to_string(i) + "]", out, sort_key);
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
 * @param v         JSON 值
 * @param indent    每级缩进空格数
 * @param level     当前嵌套层级（根节点为 0）
 * @param out       输出流
 * @param sort_key  是否对对象 key 按字典序排序
 */
static void dumpPretty(const json::Value& v, int indent, int level,
                       std::ostream& out, bool sort_key) {
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
            dumpPretty(arr[i], indent, level + 1, out, sort_key);
            if (i + 1 < arr.size()) out << ",";
            out << "\n";
        }
        out << pad << "]";
    } else if (v.is_object()) {
        const auto& obj = v.as_object();
        if (obj.empty()) { out << "{}"; return; }
        auto entries = sortedKeys(obj, sort_key);
        out << "{\n";
        size_t idx = 0;
        for (const auto& [key, val] : entries) {
            out << pad1 << json::Value(key).dump() << ": ";
            dumpPretty(val.get(), indent, level + 1, out, sort_key);
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
 * @param content   JSON 文本内容
 * @param sort_key  是否对对象 key 按字典序排序
 * @return 0 成功，非 0 失败
 */
static int compact(const std::string& content, bool sort_key,
                   const std::string& key_filter) {
    try {
        json::Value root = json::parse(content);
        filterByKey(root, key_filter);
        if (sort_key) sortObjectKeys(root);
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
 * @param content   JSON 文本内容
 * @param indent    每级缩进空格数
 * @param sort_key  是否对对象 key 按字典序排序
 * @return 0 成功，非 0 失败
 */
static int formatJson(const std::string& content, int indent, bool sort_key,
                      const std::string& key_filter) {
    try {
        json::Value root = json::parse(content);
        filterByKey(root, key_filter);
        if (sort_key) sortObjectKeys(root);
        dumpPretty(root, indent, 0, std::cout, sort_key);
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

/**
 * @brief 处理 JSON 字符串，支持排序、格式化、压缩、过滤、校验
 *
 * 复用已有的解析、过滤、排序、格式化逻辑，结果以字符串返回，
 * 供命令行和 WASM 环境共用。
 *
 * @param content       JSON 文本
 * @param sort_key      是否对对象 key 按字典序排序 (-s)
 * @param flag_format   是否格式化输出 (-F)
 * @param flag_compact  是否压缩为单行 (-c)
 * @param indent_spaces 每级缩进空格数 (-n，默认 2)
 * @param key_filter    key 过滤通配符模式 (-k，空串不过滤)
 * @param flag_validate 是否仅校验 (-v)
 * @return 处理结果字符串
 */
std::string process_json(const std::string& content,
                         bool sort_key,
                         bool flag_format,
                         bool flag_compact,
                         int indent_spaces,
                         const std::string& key_filter,
                         bool flag_validate) {
    try {
        if (flag_validate) {
            json::parse(content);
            return "JSON validation passed.";
        }
        json::Value root = json::parse(content);
        filterByKey(root, key_filter);
        if (sort_key) sortObjectKeys(root);
        std::ostringstream oss;
        if (flag_format) {
            if (indent_spaces <= 0) indent_spaces = 2;
            dumpPretty(root, indent_spaces, 0, oss, sort_key);
        } else {
            // 默认或 -c 均输出紧凑格式
            oss << root.dump();
        }
        return oss.str();
    } catch (const json::ParseError& e) {
        return "Error: [line " + std::to_string(e.line()) +
               ", col " + std::to_string(e.column()) + "] " + e.what();
    } catch (const std::exception& e) {
        return std::string("Error: ") + e.what();
    }
}

#ifndef __EMSCRIPTEN__
int main(int argc, char* argv[]) {
    // 解析命令行参数
    std::string input_file;
    bool flag_validate = false;
    bool flag_compact  = false;
    bool flag_format   = false;
    bool flag_print    = false;
    bool flag_sort_key = false;
    std::string key_filter;
    int  indent_spaces = 2;
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if ((arg == "-f" || arg == "-i") && i + 1 < argc) {
            input_file = argv[++i];
        } else if (arg == "-v") {
            flag_validate = true;
        } else if (arg == "-c") {
            flag_compact = true;
        } else if (arg == "-F") {
            flag_format = true;
        } else if (arg == "-p") {
            flag_print = true;
        } else if (arg == "-s") {
            flag_sort_key = true;
        } else if (arg == "-k" && i + 1 < argc) {
            key_filter = argv[++i];
        } else if (arg == "-n" && i + 1 < argc) {
            indent_spaces = std::atoi(argv[++i]);
            if (indent_spaces <= 0) indent_spaces = 2;
        }
    }

    if (input_file.empty()) {
        std::cerr << "Usage: " << argv[0] << " -f <json_file> [-v] [-c] [-F [-n N]] [-p] [-s] [-k pattern]\n"
                  << "  -f  input JSON file\n"
                  << "  -v  validate only (no traversal)\n"
                  << "  -c  compact to single line\n"
                  << "  -F  pretty-print with indentation\n"
                  << "  -n  spaces per indent level (default 2, use with -F)\n"
                  << "  -p  print raw file content\n"
                  << "  -s  sort object keys in output\n"
                  << "  -k  key filter pattern (wildcards: * and ?)\n";
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

    // 压缩模式：输出单行紧凑 JSON（-s 时 key 按字典序排序）
    if (flag_compact) {
        return compact(content, flag_sort_key, key_filter);
    }

    // 格式化模式：压缩后重新按缩进输出（-s 时 key 按字典序排序）
    if (flag_format) {
        return formatJson(content, indent_spaces, flag_sort_key, key_filter);
    }

    // 原样输出模式：直接输出文件内容
    if (flag_print) {
        std::cout << content;
        return 0;
    }

    // 默认模式：解析 & 遍历输出（-s 时 key 按字典序排序）
    try {
        json::Value root = json::parse(content);
        filterByKey(root, key_filter);
        traverse(root, "", std::cout, flag_sort_key);
    } catch (const json::ParseError& e) {
        std::cerr << "JSON parse error: " << e.what() << "\n";
        return EXIT_PARSE;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
#endif  // __EMSCRIPTEN__

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
EMSCRIPTEN_BINDINGS(json_cmd) {
    emscripten::function("process_json", &process_json);
}
#endif  // __EMSCRIPTEN__
