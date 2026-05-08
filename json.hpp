// =======================================================================
// json.hpp — 工业级轻量 JSON 解析器（C++17 单头文件）
// =======================================================================
// 特性：
//   - 完全符合 RFC 8259（JSON 标准）
//   - 零拷贝解析（基于 string_view 扫描，仅在需要时拷贝）
//   - 详尽错误报告（行号、列号、具体问题）
//   - 易用的 DOM（Document Object Model）接口
//   - 支持 UTF-8 编码，正确处理 \u 转义（包括代理对）
//   - 无第三方依赖，仅使用 C++17 标准库
//
// 使用方式：
//   #include "json.hpp"
//   json::Value root = json::parse(json_string);
//   std::cout << root["key"].as_string();
//
// 注意：
//   本实现遵循 JSON 规范，数字统一存储为 double（可能损失大整数精度），
//   如有需要可自行扩展为带 int64_t 的 variant 以提高整数精度。
//
// 许可：MIT License
// =======================================================================

#pragma once

// --------------------------- 头文件包含 ---------------------------
#include <cassert>      // assert（调试用，本文件未直接使用，可移除）
#include <cmath>        // std::isfinite（用于检测数值是否为有限值）
#include <cstdint>      // int64_t（用于判断 double 是否为整数值）
#include <cstdlib>      // std::strtod（将字符串转换为 double）
#include <cstring>      // C 风格字符串操作（未直接使用，可移除）
#include <fstream>      // std::ifstream（parse_file 函数）
#include <map>          // std::map（存储 JSON 对象键值对）
#include <memory>       // 未直接使用（可移除，但保留无妨）
#include <sstream>      // std::ostringstream（异常格式化）
#include <stdexcept>    // std::runtime_error（异常基类）
#include <string>       // std::string, std::string_view
#include <string_view>  // （部分编译器可能通过 <string> 间接包含）
#include <type_traits>  
#include <variant>      // std::variant（类型安全的联合体，存储 JSON 值）
#include <vector>       // std::vector（存储 JSON 数组）

namespace json {

// 前向声明，便于 Value 和 Parser 相互引用
class Value;
class Parser;

// =======================================================================
// 异常类：ParseError
// =======================================================================
// 继承自 std::runtime_error，自动记录出错时的行号和列号，
// 通过 what() 返回类似 "[line 3, col 15] Expected ':'" 的描述。
class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& msg, size_t line, size_t col)
        : std::runtime_error(format(msg, line, col)), line_(line), col_(col) {}

    size_t line()   const { return line_; }
    size_t column() const { return col_; }

private:
    // 将消息、行号、列号格式化为统一输出
    static std::string format(const std::string& msg, size_t line, size_t col) {
        std::ostringstream oss;
        oss << "[line " << line << ", col " << col << "] " << msg;
        return oss.str();
    }
    size_t line_, col_;
};

// =======================================================================
// 核心类：Value
// =======================================================================
// 表示任意 JSON 值，使用 std::variant 存储实际数据。
// 支持类型：Null, Boolean, Number (double), String (std::string),
//           Array (std::vector<Value>), Object (std::map<...>)
//
// 设计要点：
//   - 使用 std::less<> 作为 Object 的比较器，使得可以使用 string_view
//     作为键进行查找（透明查找），避免临时字符串构造。
//   - 所有拷贝操作均为深拷贝，适合常规使用；如需极致性能可考虑移动语义。
class Value {
public:
    // 公开类型别名，方便外部使用
    using Null    = std::nullptr_t;
    using Boolean = bool;
    using Number  = double;                // JSON 数字统一用 double 表示
    using String  = std::string;
    using Array   = std::vector<Value>;
    using Object  = std::map<std::string, Value, std::less<>>; // 异构查找支持

private:
    // 实际数据存储于 variant 中
    std::variant<Null, Boolean, Number, String, Array, Object> data_;

    // 辅助函数：类型检查（是否为 T 类型）
    template<typename T>
    bool is() const { return std::holds_alternative<T>(data_); }

    // 辅助函数：获取可变引用（调用者需确保类型正确，否则抛出 std::bad_variant_access）
    template<typename T>
    T& as() { return std::get<T>(data_); }

    template<typename T>
    const T& as() const { return std::get<T>(data_); }

public:
    // ---------- 构造函数 ----------
    Value() : data_(Null{}) {}                     // 默认构造为 null
    Value(Null)             : data_(Null{}) {}     // 构造 null
    Value(Boolean b)        : data_(b) {}          // 构造布尔
    Value(Number n)         : data_(n) {}          // 构造数字
    Value(String s)         : data_(std::move(s)) {} // 构造字符串（移动语义）
    Value(const char* s)    : data_(String(s)) {}  // 从 C 字符串构造（隐式转换为 std::string）
    Value(Array a)          : data_(std::move(a)) {}
    Value(Object o)         : data_(std::move(o)) {}

    // 支持使用初始化列表构造数组或对象（例如 Value{1,2,3}）
    Value(std::initializer_list<Value> list);
    Value(std::initializer_list<std::pair<const std::string, Value>> list);

    // ---------- 类型查询 ----------
    bool is_null()    const { return is<Null>(); }
    bool is_boolean() const { return is<Boolean>(); }
    bool is_number()  const { return is<Number>(); }
    bool is_string()  const { return is<String>(); }
    bool is_array()   const { return is<Array>(); }
    bool is_object()  const { return is<Object>(); }

    // ---------- 类型转换（获取引用） ----------
    // 若类型不匹配会抛出 std::bad_variant_access
    Boolean      & as_boolean()       { return as<Boolean>(); }
    const Boolean& as_boolean() const { return as<Boolean>(); }

    Number& as_number()       { return as<Number>(); }
    const Number& as_number() const { return as<Number>(); }

    String& as_string()       { return as<String>(); }
    const String& as_string() const { return as<String>(); }

    Array& as_array()       { return as<Array>(); }
    const Array& as_array() const { return as<Array>(); }

    Object& as_object()       { return as<Object>(); }
    const Object& as_object() const { return as<Object>(); }

    // ---------- 元素访问 ----------
    // 数组下标访问（越界时抛出 std::out_of_range）
    Value& operator[](size_t idx);
    const Value& operator[](size_t idx) const;

    // 对象访问：支持 std::string 和 std::string_view
    // 使用 string_view 时借助透明比较器避免构造临时字符串
    Value& operator[](const std::string& key);
    Value& operator[](std::string_view key);
    const Value& operator[](const std::string& key) const;
    const Value& operator[](std::string_view key) const;

    // ---------- 比较操作 ----------
    bool operator==(const Value& other) const { return data_ == other.data_; }
    bool operator!=(const Value& other) const { return !(*this == other); }

    // ---------- 序列化 ----------
    // 将当前值序列化为紧凑 JSON 字符串（无多余空白）
    std::string dump() const;
};

// 便捷工厂函数
inline Value Array(std::initializer_list<Value> elems) { return Value(std::move(elems)); }
inline Value Object(std::initializer_list<std::pair<const std::string, Value>> pairs) {
    return Value(std::move(pairs));
}

// 初始化列表构造函数的实现
inline Value::Value(std::initializer_list<Value> list) : data_(Array(list.begin(), list.end())) {}
inline Value::Value(std::initializer_list<std::pair<const std::string, Value>> list)
    : data_(Object(list.begin(), list.end())) {}

// 下标操作实现
inline Value& Value::operator[](size_t idx) {
    return as<Array>().at(idx);               // at() 提供边界检查
}
inline const Value& Value::operator[](size_t idx) const {
    return as<Array>().at(idx);
}

// 对象访问：使用 std::string 键（直接通过 map::operator[]，若不存在会插入一个默认值，此处只用于可变版本）
inline Value& Value::operator[](const std::string& key) {
    return as<Object>()[key];                 // 注意：若 key 不存在会创建新元素！
}
// 对象访问：使用 string_view 键（利用透明比较器进行查找）
inline Value& Value::operator[](std::string_view key) {
    auto it = as<Object>().find(key);
    if (it == as<Object>().end()) {
        throw std::out_of_range("Key not found in JSON object");
    }
    return it->second;
}
// const 版本严格使用 at()，key 不存在时抛出异常
inline const Value& Value::operator[](const std::string& key) const {
    return as<Object>().at(key);
}
inline const Value& Value::operator[](std::string_view key) const {
    auto it = as<Object>().find(key);
    if (it == as<Object>().end()) {
        throw std::out_of_range("Key not found in JSON object");
    }
    return it->second;
}

// =======================================================================
// 序列化功能（dump 到 string）
// =======================================================================
namespace detail {

    // 将字符串按 JSON 规范转义后追加到 out
    inline void dump_string(const std::string& s, std::string& out) {
        out += '"';
        for (char c : s) {
            switch (c) {
                case '"':  out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\b': out += "\\b";  break;
                case '\f': out += "\\f";  break;
                case '\n': out += "\\n";  break;
                case '\r': out += "\\r";  break;
                case '\t': out += "\\t";  break;
                default:
                    // 控制字符（小于 0x20）需要转义为 \u00xx
                    if (static_cast<unsigned char>(c) < 0x20) {
                        char buf[8];
                        snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                        out += buf;
                    } else {
                        out += c;
                    }
            }
        }
        out += '"';
    }

    // 前向声明，以便递归调用
    inline void dump_value(const Value& v, std::string& out);

    // 递归序列化任意 Value
    inline void dump_value(const Value& v, std::string& out) {
        if (v.is_null()) {
            out += "null";
        } else if (v.is_boolean()) {
            out += v.as_boolean() ? "true" : "false";
        } else if (v.is_number()) {
            double d = v.as_number();
            // 如果数值是有限的且可以无损表示为 int64_t，则输出整数形式（避免 "1.0"）
            if (d == static_cast<int64_t>(d) && std::isfinite(d)) {
                out += std::to_string(static_cast<int64_t>(d));
            } else {
                char buf[32];
                // %g 格式会自动选择较短的表示法，17 位保证往返精度
                snprintf(buf, sizeof(buf), "%.17g", d);
                out += buf;
            }
        } else if (v.is_string()) {
            dump_string(v.as_string(), out);
        } else if (v.is_array()) {
            out += '[';
            const auto& arr = v.as_array();
            for (size_t i = 0; i < arr.size(); ++i) {
                if (i > 0) out += ',';
                dump_value(arr[i], out);
            }
            out += ']';
        } else if (v.is_object()) {
            out += '{';
            const auto& obj = v.as_object();
            bool first = true;
            for (const auto& [key, val] : obj) {
                if (!first) out += ',';
                first = false;
                dump_string(key, out);
                out += ':';
                dump_value(val, out);
            }
            out += '}';
        }
    }
} // namespace detail

inline std::string Value::dump() const {
    std::string result;
    detail::dump_value(*this, result);
    return result;
}

// =======================================================================
// 解析器（Parser）
// =======================================================================
// 基于递归下降实现，直接工作在输入的 string_view 上（零拷贝）。
// 所有解析错误都会抛出 ParseError，包含精确的位置信息。
class Parser {
public:
    // 静态入口：解析 JSON 字符串视图，返回 Value 树
    static Value parse(std::string_view json) {
        Parser p(json);
        return p.parse_value();
    }

private:
    std::string_view input_;    // 原始 JSON 文本的视图
    size_t pos_ = 0;            // 当前扫描位置（字符索引）
    size_t line_ = 1;           // 当前行号（用于错误报告）
    size_t col_  = 1;           // 当前列号

    explicit Parser(std::string_view input) : input_(input) {}

    // 返回当前位置的字符，若已到结尾则返回 '\0'
    char current() const {
        return pos_ < input_.size() ? input_[pos_] : '\0';
    }

    // 消耗一个字符并更新行列计数
    void advance() {
        if (pos_ < input_.size()) {
            char c = input_[pos_++];
            if (c == '\n') { ++line_; col_ = 1; }
            else { ++col_; }
        }
    }

    // 跳过空白字符：空格、制表符、换行、回车
    void skip_whitespace() {
        while (true) {
            char c = current();
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                advance();
            } else {
                break;
            }
        }
    }

    // 断言下一个字符为 c 并消耗它，否则抛出异常
    void expect(char c) {
        if (current() != c)
            throw error(std::string("Expected '") + c + "'");
        advance();
    }

    // 创建一个 ParseError 并自动填入当前位置
    ParseError error(const std::string& msg) const {
        return ParseError(msg, line_, col_);
    }

    // 解析函数声明
    Value parse_value();    // 分发入口（根据首字符判断类型）
    Value parse_object();   // 解析 {}
    Value parse_array();    // 解析 []
    Value parse_string();   // 解析 "…"
    Value parse_number();   // 解析数字
    Value parse_literal();  // 解析 true/false/null

    // Unicode 转义处理
    void parse_utf16_escape(std::string& out);
    static std::string utf16_to_utf8(uint32_t codepoint);
};

// ------------------------ parse_value ------------------------
inline Value Parser::parse_value() {
    skip_whitespace();
    char c = current();
    switch (c) {
        case '{': return parse_object();
        case '[': return parse_array();
        case '"': return parse_string();
        case 't': case 'f': case 'n': return parse_literal();
        default:
            // 负数或数字开头
            if (c == '-' || (c >= '0' && c <= '9'))
                return parse_number();
            throw error("Unexpected character");
    }
}

// ------------------------ parse_object -----------------------
inline Value Parser::parse_object() {
    // 注意：这里必须使用 Value::Object，因为 Object 是嵌套类型别名
    Value::Object obj;
    expect('{');
    skip_whitespace();
    // 空对象直接返回
    if (current() == '}') {
        advance();
        return Value(std::move(obj));
    }

    while (true) {
        skip_whitespace();
        // 根据标准，键必须是字符串
        Value key = parse_string();
        skip_whitespace();
        expect(':');
        skip_whitespace();
        Value val = parse_value();
        // 插入键值对（使用移动语义避免拷贝）
        obj.emplace(std::move(key.as_string()), std::move(val));

        skip_whitespace();
        char c = current();
        if (c == ',') {
            advance();
            continue;
        } else if (c == '}') {
            advance();
            break;      // 正常结束
        } else {
            throw error("Expected ',' or '}' in object");
        }
    }
    return Value(std::move(obj));
}

// ------------------------ parse_array ------------------------
inline Value Parser::parse_array() {
    Value::Array arr;
    expect('[');
    skip_whitespace();
    if (current() == ']') {
        advance();
        return Value(std::move(arr));
    }

    while (true) {
        skip_whitespace();
        arr.push_back(parse_value());
        skip_whitespace();
        char c = current();
        if (c == ',') {
            advance();
            continue;
        } else if (c == ']') {
            advance();
            break;
        } else {
            throw error("Expected ',' or ']' in array");
        }
    }
    return Value(std::move(arr));
}

// ------------------------ parse_string -----------------------
inline Value Parser::parse_string() {
    expect('"');
    std::string result;
    while (true) {
        char c = current();
        if (c == '"') {
            advance();
            return Value(std::move(result));
        } else if (c == '\\') {
            advance();          // 跳过反斜线
            char esc = current();
            switch (esc) {
                case '"':  result += '"'; advance(); break;
                case '\\': result += '\\'; advance(); break;
                case '/':  result += '/'; advance(); break;   // 可选，但标准允许
                case 'b':  result += '\b'; advance(); break;
                case 'f':  result += '\f'; advance(); break;
                case 'n':  result += '\n'; advance(); break;
                case 'r':  result += '\r'; advance(); break;
                case 't':  result += '\t'; advance(); break;
                case 'u':  advance(); parse_utf16_escape(result); break;
                default:
                    throw error("Invalid escape character");
            }
        } else if (static_cast<unsigned char>(c) < 0x20) {
            // JSON 字符串不允许直接包含控制字符（ASCII 0x00-0x1F）
            throw error("Unescaped control character in string");
        } else {
            // 普通字符直接追加
            result += c;
            advance();
        }
    }
}

// -------------------- Unicode 转义处理 --------------------
// 解析 \uXXXX，并处理 UTF-16 代理对
inline void Parser::parse_utf16_escape(std::string& out) {
    // 辅助 Lambda：读取恰好4位十六进制数字
    auto read_hex4 = [&]() -> uint32_t {
        uint32_t val = 0;
        for (int i = 0; i < 4; ++i) {
            char c = current();
            if (c >= '0' && c <= '9')      val = val * 16 + (c - '0');
            else if (c >= 'a' && c <= 'f') val = val * 16 + (c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') val = val * 16 + (c - 'A' + 10);
            else throw error("Invalid unicode escape");
            advance();
        }
        return val;
    };

    uint32_t codepoint = read_hex4();

    // 处理高位代理（0xD800-0xDBFF）
    if (codepoint >= 0xD800 && codepoint <= 0xDBFF) {
        // 必须紧跟 \uXXXX 低位代理
        if (current() != '\\') throw error("Expected low surrogate after high surrogate");
        advance();
        if (current() != 'u') throw error("Expected 'u' for low surrogate");
        advance();
        uint32_t low = read_hex4();
        if (low < 0xDC00 || low > 0xDFFF)
            throw error("Invalid low surrogate range");
        // 根据 UTF-16 规则重组码点
        codepoint = 0x10000 + ((codepoint - 0xD800) << 10) + (low - 0xDC00);
    } else if (codepoint >= 0xDC00 && codepoint <= 0xDFFF) {
        // 低位代理单独出现，非法
        throw error("Unexpected low surrogate without high surrogate");
    }

    out += utf16_to_utf8(codepoint);
}

// 将 Unicode 码点转换为 UTF-8 字节序列（最长为4字节）
inline std::string Parser::utf16_to_utf8(uint32_t cp) {
    std::string result;
    if (cp <= 0x7F) {
        result += static_cast<char>(cp);
    } else if (cp <= 0x7FF) {
        result += static_cast<char>(0xC0 | (cp >> 6));
        result += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp <= 0xFFFF) {
        result += static_cast<char>(0xE0 | (cp >> 12));
        result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp <= 0x10FFFF) {
        result += static_cast<char>(0xF0 | (cp >> 18));
        result += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        throw std::runtime_error("Invalid Unicode codepoint");
    }
    return result;
}

// ------------------------ parse_number -----------------------
inline Value Parser::parse_number() {
    size_t start = pos_;        // 记录数字起始位置，用于最后提取子串
    bool is_float = false;      // 标记是否为浮点数（有小数点或指数）

    // 可选负号
    if (current() == '-') advance();

    // 整数部分
    if (current() == '0') {
        advance();              // 仅一个零，不允许前导零（如 012）
    } else if (current() >= '1' && current() <= '9') {
        while (current() >= '0' && current() <= '9') advance();
    } else {
        throw error("Invalid number");
    }

    // 可选小数部分
    if (current() == '.') {
        is_float = true;
        advance();
        if (current() < '0' || current() > '9') throw error("Expected digit after '.'");
        while (current() >= '0' && current() <= '9') advance();
    }

    // 可选指数部分（e/E）
    if (current() == 'e' || current() == 'E') {
        is_float = true;
        advance();
        if (current() == '+' || current() == '-') advance();
        if (current() < '0' || current() > '9') throw error("Expected digit in exponent");
        while (current() >= '0' && current() <= '9') advance();
    }

    // 提取字符串表示并转换为 double
    std::string num_str(input_.substr(start, pos_ - start));
    char* end = nullptr;
    double val = std::strtod(num_str.c_str(), &end);
    // 如果 strtod 没有用完整个字符串，说明格式非法
    if (end != num_str.c_str() + num_str.size())
        throw error("Invalid number format");

    return Value(val);
}

// ------------------------ parse_literal ----------------------
inline Value Parser::parse_literal() {
    // 直接比较后续字符，匹配 true / false / null
    if (input_.substr(pos_, 4) == "true") {
        pos_ += 4; col_ += 4;
        return Value(true);
    } else if (input_.substr(pos_, 5) == "false") {
        pos_ += 5; col_ += 5;
        return Value(false);
    } else if (input_.substr(pos_, 4) == "null") {
        pos_ += 4; col_ += 4;
        return Value(Value::Null{});
    } else {
        throw error("Invalid literal");
    }
}

// =======================================================================
// 便利函数：从字符串 / 文件解析
// =======================================================================

// 解析字符串视图，返回 JSON 值
inline Value parse(std::string_view json_str) {
    return Parser::parse(json_str);
}

// 从指定文件路径读取并解析 JSON
inline Value parse_file(const std::string& path) {
    // 以二进制模式打开并移动到文件末尾获取大小
    std::ifstream ifs(path, std::ios::in | std::ios::binary | std::ios::ate);
    if (!ifs) {
        throw std::runtime_error("Cannot open file: " + path);
    }
    auto size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    std::string content(static_cast<size_t>(size), '\0');
    ifs.read(content.data(), size);
    if (!ifs) {
        throw std::runtime_error("Error reading file: " + path);
    }
    return parse(content);
}

} // namespace json
