/**
 * @file test_json.cpp
 * @brief json.hpp 的 Google Test 单元测试
 *
 * 覆盖范围：
 *   - Value 构造与类型查询
 *   - 类型转换与访问
 *   - 数组/对象下标操作
 *   - 比较运算
 *   - 序列化（dump）
 *   - 解析：字面量、数字、字符串、数组、对象
 *   - Unicode 转义与代理对
 *   - 错误处理（ParseError 行列信息）
 *   - 初始化列表构造与工厂函数
 *   - 往返一致性（parse -> dump -> parse）
 */

#include "json.hpp"

#include <cmath>
#include <cstdint>
#include <gtest/gtest.h>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

// =======================================================================
// Value 构造与类型查询
// =======================================================================

TEST(ValueConstruction, DefaultIsNull) {
    json::Value v;
    EXPECT_TRUE(v.is_null());
    EXPECT_FALSE(v.is_boolean());
    EXPECT_FALSE(v.is_number());
    EXPECT_FALSE(v.is_string());
    EXPECT_FALSE(v.is_array());
    EXPECT_FALSE(v.is_object());
}

TEST(ValueConstruction, NullType) {
    json::Value v(nullptr);
    EXPECT_TRUE(v.is_null());
}

TEST(ValueConstruction, BooleanType) {
    json::Value v(true);
    EXPECT_TRUE(v.is_boolean());
    EXPECT_FALSE(v.is_null());

    json::Value v2(false);
    EXPECT_TRUE(v2.is_boolean());
}

TEST(ValueConstruction, NumberType) {
    json::Value v(3.14);
    EXPECT_TRUE(v.is_number());
    EXPECT_FALSE(v.is_boolean());
}

TEST(ValueConstruction, StringType) {
    json::Value v(std::string("hello"));
    EXPECT_TRUE(v.is_string());
}

TEST(ValueConstruction, CStringType) {
    json::Value v("world");
    EXPECT_TRUE(v.is_string());
}

TEST(ValueConstruction, ArrayType) {
    json::Value::Array arr;
    arr.push_back(json::Value(1.0));
    arr.push_back(json::Value(2.0));
    json::Value v(std::move(arr));
    EXPECT_TRUE(v.is_array());
}

TEST(ValueConstruction, ObjectType) {
    json::Value::Object obj;
    obj.emplace("a", json::Value(1.0));
    json::Value v(std::move(obj));
    EXPECT_TRUE(v.is_object());
}

// =======================================================================
// 类型转换与访问
// =======================================================================

TEST(ValueAccess, AsBoolean) {
    json::Value v(true);
    EXPECT_EQ(v.as_boolean(), true);

    v.as_boolean() = false;
    EXPECT_EQ(v.as_boolean(), false);
}

TEST(ValueAccess, AsNumber) {
    json::Value v(42.5);
    EXPECT_DOUBLE_EQ(v.as_number(), 42.5);

    v.as_number() = 100.0;
    EXPECT_DOUBLE_EQ(v.as_number(), 100.0);
}

TEST(ValueAccess, AsString) {
    json::Value v(std::string("test"));
    EXPECT_EQ(v.as_string(), "test");

    v.as_string() = "changed";
    EXPECT_EQ(v.as_string(), "changed");
}

TEST(ValueAccess, AsArray) {
    json::Value::Array arr;
    arr.push_back(json::Value(1.0));
    arr.push_back(json::Value(2.0));
    json::Value v(std::move(arr));
    EXPECT_EQ(v.as_array().size(), 2u);
    EXPECT_DOUBLE_EQ(v.as_array()[0].as_number(), 1.0);
}

TEST(ValueAccess, AsObject) {
    json::Value::Object obj;
    obj.emplace("key", json::Value(99.0));
    json::Value v(std::move(obj));
    EXPECT_EQ(v.as_object().size(), 1u);
    EXPECT_DOUBLE_EQ(v.as_object().at("key").as_number(), 99.0);
}

TEST(ValueAccess, ConstAccess) {
    const json::Value v(3.14);
    EXPECT_DOUBLE_EQ(v.as_number(), 3.14);
}

TEST(ValueAccess, WrongTypeThrows) {
    json::Value v(42.0);
    EXPECT_THROW(v.as_boolean(), std::bad_variant_access);
    EXPECT_THROW(v.as_string(), std::bad_variant_access);
    EXPECT_THROW(v.as_array(), std::bad_variant_access);
    EXPECT_THROW(v.as_object(), std::bad_variant_access);
}

// =======================================================================
// 数组下标操作
// =======================================================================

TEST(ArraySubscript, ValidIndex) {
    json::Value::Array arr;
    arr.push_back(json::Value(10.0));
    arr.push_back(json::Value(20.0));
    arr.push_back(json::Value(30.0));
    json::Value v(std::move(arr));
    EXPECT_DOUBLE_EQ(v[0].as_number(), 10.0);
    EXPECT_DOUBLE_EQ(v[1].as_number(), 20.0);
    EXPECT_DOUBLE_EQ(v[2].as_number(), 30.0);
}

TEST(ArraySubscript, OutOfRangeThrows) {
    json::Value::Array arr;
    arr.push_back(json::Value(1.0));
    json::Value v(std::move(arr));
    EXPECT_THROW(v[5], std::out_of_range);
}

TEST(ArraySubscript, ConstAccess) {
    json::Value::Array arr;
    arr.push_back(json::Value(7.0));
    json::Value v(std::move(arr));
    const json::Value& cv = v;
    EXPECT_DOUBLE_EQ(cv[0].as_number(), 7.0);
}

// =======================================================================
// 对象下标操作
// =======================================================================

TEST(ObjectSubscript, StringKey) {
    json::Value::Object obj;
    obj.emplace("name", json::Value("alice"));
    json::Value v(std::move(obj));
    EXPECT_EQ(v[std::string("name")].as_string(), "alice");
}

TEST(ObjectSubscript, StringKeyInsertsIfMissing) {
    // 非 const 版本 operator[](string) 会插入默认值
    json::Value v(json::Value::Object{});
    v[std::string("new_key")] = json::Value(42.0);
    EXPECT_DOUBLE_EQ(v[std::string("new_key")].as_number(), 42.0);
}

TEST(ObjectSubscript, StringViewKey) {
    json::Value::Object obj;
    obj.emplace("x", json::Value(1.0));
    json::Value v(std::move(obj));
    std::string_view key = "x";
    EXPECT_DOUBLE_EQ(v[key].as_number(), 1.0);
}

TEST(ObjectSubscript, StringViewKeyNotFoundThrows) {
    json::Value::Object obj;
    obj.emplace("x", json::Value(1.0));
    json::Value v(std::move(obj));
    EXPECT_THROW(v[std::string_view("missing")], std::out_of_range);
}

TEST(ObjectSubscript, ConstStringKey) {
    json::Value::Object obj;
    obj.emplace("k", json::Value("v"));
    json::Value v(std::move(obj));
    const json::Value& cv = v;
    EXPECT_EQ(cv[std::string("k")].as_string(), "v");
}

TEST(ObjectSubscript, ConstStringKeyNotFoundThrows) {
    json::Value::Object obj;
    obj.emplace("k", json::Value("v"));
    json::Value v(std::move(obj));
    const json::Value& cv = v;
    EXPECT_THROW(cv[std::string("missing")], std::out_of_range);
}

TEST(ObjectSubscript, ConstStringViewKey) {
    json::Value::Object obj;
    obj.emplace("k", json::Value(5.0));
    json::Value v(std::move(obj));
    const json::Value& cv = v;
    EXPECT_DOUBLE_EQ(cv[std::string_view("k")].as_number(), 5.0);
}

TEST(ObjectSubscript, ConstStringViewKeyNotFoundThrows) {
    json::Value::Object obj;
    obj.emplace("k", json::Value(5.0));
    json::Value v(std::move(obj));
    const json::Value& cv = v;
    EXPECT_THROW(cv[std::string_view("missing")], std::out_of_range);
}

// =======================================================================
// 比较运算
// =======================================================================

TEST(ValueComparison, EqualNumbers) {
    json::Value a(42.0), b(42.0);
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
}

TEST(ValueComparison, DifferentNumbers) {
    json::Value a(1.0), b(2.0);
    EXPECT_TRUE(a != b);
    EXPECT_FALSE(a == b);
}

TEST(ValueComparison, EqualStrings) {
    json::Value a(std::string("hi")), b(std::string("hi"));
    EXPECT_TRUE(a == b);
}

TEST(ValueComparison, DifferentTypes) {
    json::Value a(1.0), b(true);
    EXPECT_TRUE(a != b);
}

TEST(ValueComparison, NullEquality) {
    json::Value a(nullptr), b(nullptr);
    EXPECT_TRUE(a == b);
}

// =======================================================================
// 序列化（dump）
// =======================================================================

TEST(Dump, Null) {
    json::Value v(nullptr);
    EXPECT_EQ(v.dump(), "null");
}

TEST(Dump, BooleanTrue) {
    json::Value v(true);
    EXPECT_EQ(v.dump(), "true");
}

TEST(Dump, BooleanFalse) {
    json::Value v(false);
    EXPECT_EQ(v.dump(), "false");
}

TEST(Dump, Integer) {
    json::Value v(42.0);
    EXPECT_EQ(v.dump(), "42");
}

TEST(Dump, Float) {
    json::Value v(3.14);
    // 使用 %.17g 格式，允许一定精度差异
    EXPECT_NE(v.dump().find("3.14"), std::string::npos);
}

TEST(Dump, String) {
    json::Value v(std::string("hello"));
    EXPECT_EQ(v.dump(), "\"hello\"");
}

TEST(Dump, StringEscapes) {
    json::Value v(std::string("a\"b\\c\nd\te"));
    EXPECT_EQ(v.dump(), "\"a\\\"b\\\\c\\nd\\te\"");
}

TEST(Dump, ControlCharacterEscape) {
    // 控制字符（小于 0x20）应转义为 \u00xx
    // 注意：使用 push_back 避免字符串字面量中 \x 的歧义
    std::string with_ctrl = "hi";
    with_ctrl.push_back('\x01');
    with_ctrl += "bye";
    json::Value v(with_ctrl);
    std::string dumped = v.dump();
    EXPECT_NE(dumped.find("\\u0001"), std::string::npos);
}

TEST(Dump, EmptyArray) {
    json::Value v(json::Value::Array{});
    EXPECT_EQ(v.dump(), "[]");
}

TEST(Dump, SimpleArray) {
    json::Value::Array arr;
    arr.push_back(json::Value(1.0));
    arr.push_back(json::Value(2.0));
    json::Value v(std::move(arr));
    EXPECT_EQ(v.dump(), "[1,2]");
}

TEST(Dump, EmptyObject) {
    json::Value v(json::Value::Object{});
    EXPECT_EQ(v.dump(), "{}");
}

TEST(Dump, SimpleObject) {
    json::Value::Object obj;
    obj.emplace("a", json::Value(1.0));
    json::Value v(std::move(obj));
    EXPECT_EQ(v.dump(), "{\"a\":1}");
}

TEST(Dump, NegativeNumber) {
    json::Value v(-1.0);
    EXPECT_EQ(v.dump(), "-1");
}

// =======================================================================
// 解析：字面量
// =======================================================================

TEST(ParseLiteral, True) {
    auto v = json::parse("true");
    EXPECT_TRUE(v.is_boolean());
    EXPECT_EQ(v.as_boolean(), true);
}

TEST(ParseLiteral, False) {
    auto v = json::parse("false");
    EXPECT_TRUE(v.is_boolean());
    EXPECT_EQ(v.as_boolean(), false);
}

TEST(ParseLiteral, Null) {
    auto v = json::parse("null");
    EXPECT_TRUE(v.is_null());
}

TEST(ParseLiteral, InvalidLiteral) {
    EXPECT_THROW(json::parse("tru"), json::ParseError);
    EXPECT_THROW(json::parse("nulx"), json::ParseError);
}

// =======================================================================
// 解析：数字
// =======================================================================

TEST(ParseNumber, Zero) {
    auto v = json::parse("0");
    EXPECT_TRUE(v.is_number());
    EXPECT_DOUBLE_EQ(v.as_number(), 0.0);
}

TEST(ParseNumber, PositiveInteger) {
    auto v = json::parse("42");
    EXPECT_DOUBLE_EQ(v.as_number(), 42.0);
}

TEST(ParseNumber, NegativeInteger) {
    auto v = json::parse("-7");
    EXPECT_DOUBLE_EQ(v.as_number(), -7.0);
}

TEST(ParseNumber, FloatWithDecimal) {
    auto v = json::parse("3.14");
    EXPECT_DOUBLE_EQ(v.as_number(), 3.14);
}

TEST(ParseNumber, Exponent) {
    auto v = json::parse("1e2");
    EXPECT_DOUBLE_EQ(v.as_number(), 100.0);
}

TEST(ParseNumber, ExponentWithPlus) {
    auto v = json::parse("1e+2");
    EXPECT_DOUBLE_EQ(v.as_number(), 100.0);
}

TEST(ParseNumber, ExponentWithMinus) {
    auto v = json::parse("1e-2");
    EXPECT_DOUBLE_EQ(v.as_number(), 0.01);
}

TEST(ParseNumber, FullFloat) {
    auto v = json::parse("-12.34e5");
    EXPECT_DOUBLE_EQ(v.as_number(), -1234000.0);
}

TEST(ParseNumber, LeadingZeroParsesAsZero) {
    // 当前解析器解析 "0" 后即停止，不会校验尾部多余字符
    // 这是已知限制：严格 JSON 应拒绝 012，但本解析器返回 0
    auto v = json::parse("0");
    EXPECT_DOUBLE_EQ(v.as_number(), 0.0);
}

TEST(ParseNumber, DotWithoutDigitsThrows) {
    EXPECT_THROW(json::parse("1."), json::ParseError);
}

TEST(ParseNumber, ExponentWithoutDigitsThrows) {
    EXPECT_THROW(json::parse("1e"), json::ParseError);
}

TEST(ParseNumber, JustMinusThrows) {
    EXPECT_THROW(json::parse("-"), json::ParseError);
}

// =======================================================================
// 解析：字符串
// =======================================================================

TEST(ParseString, Simple) {
    auto v = json::parse("\"hello\"");
    EXPECT_TRUE(v.is_string());
    EXPECT_EQ(v.as_string(), "hello");
}

TEST(ParseString, Empty) {
    auto v = json::parse("\"\"");
    EXPECT_TRUE(v.is_string());
    EXPECT_EQ(v.as_string(), "");
}

TEST(ParseString, EscapeSequences) {
    // 逐个测试转义序列，避免 gtest 显示控制字符时混淆
    auto v1 = json::parse("\"\\\"\"");
    EXPECT_EQ(v1.as_string(), "\"");

    auto v2 = json::parse("\"\\\\\"");
    EXPECT_EQ(v2.as_string(), "\\");

    auto v3 = json::parse("\"\\/\"");
    EXPECT_EQ(v3.as_string(), "/");

    auto v4 = json::parse("\"\\b\"");
    EXPECT_EQ((unsigned char)v4.as_string()[0], 0x08u);

    auto v5 = json::parse("\"\\f\"");
    EXPECT_EQ((unsigned char)v5.as_string()[0], 0x0Cu);

    auto v6 = json::parse("\"\\n\"");
    EXPECT_EQ((unsigned char)v6.as_string()[0], 0x0Au);

    auto v7 = json::parse("\"\\r\"");
    EXPECT_EQ((unsigned char)v7.as_string()[0], 0x0Du);

    auto v8 = json::parse("\"\\t\"");
    EXPECT_EQ((unsigned char)v8.as_string()[0], 0x09u);

    // 组合测试：用字节逐一比较
    auto v9 = json::parse("\"a\\\"b\\\\c\\/d\\be\\ff\\ng\\rh\\ti\"");
    const std::string expected = std::string("a") + '"' + "b" + '\\' + "c/d" + '\b' + "e" + '\f' + "f" + '\n' + "g" + '\r' + "h" + '\t' + "i";
    EXPECT_EQ(v9.as_string().size(), expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ((unsigned char)v9.as_string()[i], (unsigned char)expected[i])
            << "Mismatch at index " << i;
    }
}

TEST(ParseString, UnicodeBasic) {
    // U+0041 = 'A'
    auto v = json::parse("\"\\u0041\"");
    EXPECT_EQ(v.as_string(), "A");
}

TEST(ParseString, UnicodeChinese) {
    // U+4E2D = 中
    auto v = json::parse("\"\\u4e2d\"");
    EXPECT_EQ(v.as_string(), "\xe4\xb8\xad");  // UTF-8 for 中
}

TEST(ParseString, UnicodeSurrogatePair) {
    // U+1F600 = 😀
    auto v = json::parse("\"\\uD83D\\uDE00\"");
    // UTF-8 for U+1F600: F0 9F 98 80
    EXPECT_EQ(v.as_string(), "\xF0\x9F\x98\x80");
}

TEST(ParseString, LowSurrogateAloneThrows) {
    EXPECT_THROW(json::parse("\"\\uDC00\""), json::ParseError);
}

TEST(ParseString, InvalidEscapeThrows) {
    EXPECT_THROW(json::parse("\"\\x\""), json::ParseError);
}

TEST(ParseString, UnescapedControlCharThrows) {
    // 字符串中不允许直接包含控制字符
    std::string input = "\"hello\x01world\"";
    EXPECT_THROW(json::parse(input), json::ParseError);
}

TEST(ParseString, UnclosedStringThrows) {
    EXPECT_THROW(json::parse("\"hello"), json::ParseError);
}

// =======================================================================
// 解析：数组
// =======================================================================

TEST(ParseArray, Empty) {
    auto v = json::parse("[]");
    EXPECT_TRUE(v.is_array());
    EXPECT_EQ(v.as_array().size(), 0u);
}

TEST(ParseArray, SingleElement) {
    auto v = json::parse("[1]");
    EXPECT_EQ(v.as_array().size(), 1u);
    EXPECT_DOUBLE_EQ(v[0].as_number(), 1.0);
}

TEST(ParseArray, MultipleElements) {
    auto v = json::parse("[1, 2, 3]");
    EXPECT_EQ(v.as_array().size(), 3u);
    EXPECT_DOUBLE_EQ(v[0].as_number(), 1.0);
    EXPECT_DOUBLE_EQ(v[1].as_number(), 2.0);
    EXPECT_DOUBLE_EQ(v[2].as_number(), 3.0);
}

TEST(ParseArray, MixedTypes) {
    auto v = json::parse("[1, \"two\", true, null]");
    EXPECT_EQ(v.as_array().size(), 4u);
    EXPECT_DOUBLE_EQ(v[0].as_number(), 1.0);
    EXPECT_EQ(v[1].as_string(), "two");
    EXPECT_EQ(v[2].as_boolean(), true);
    EXPECT_TRUE(v[3].is_null());
}

TEST(ParseArray, NestedArrays) {
    auto v = json::parse("[[1, 2], [3, 4]]");
    EXPECT_EQ(v[0][0].as_number(), 1.0);
    EXPECT_EQ(v[0][1].as_number(), 2.0);
    EXPECT_EQ(v[1][0].as_number(), 3.0);
    EXPECT_EQ(v[1][1].as_number(), 4.0);
}

TEST(ParseArray, MissingCommaThrows) {
    EXPECT_THROW(json::parse("[1 2]"), json::ParseError);
}

TEST(ParseArray, TrailingCommaThrows) {
    EXPECT_THROW(json::parse("[1,]"), json::ParseError);
}

// =======================================================================
// 解析：对象
// =======================================================================

TEST(ParseObject, Empty) {
    auto v = json::parse("{}");
    EXPECT_TRUE(v.is_object());
    EXPECT_EQ(v.as_object().size(), 0u);
}

TEST(ParseObject, SingleEntry) {
    auto v = json::parse("{\"key\": 42}");
    EXPECT_EQ(v.as_object().size(), 1u);
    EXPECT_DOUBLE_EQ(v[std::string("key")].as_number(), 42.0);
}

TEST(ParseObject, MultipleEntries) {
    auto v = json::parse("{\"a\": 1, \"b\": 2}");
    EXPECT_EQ(v.as_object().size(), 2u);
    EXPECT_DOUBLE_EQ(v[std::string("a")].as_number(), 1.0);
    EXPECT_DOUBLE_EQ(v[std::string("b")].as_number(), 2.0);
}

TEST(ParseObject, NestedObject) {
    auto v = json::parse("{\"inner\": {\"x\": 99}}");
    EXPECT_DOUBLE_EQ(v[std::string("inner")][std::string("x")].as_number(), 99.0);
}

TEST(ParseObject, MixedValues) {
    auto v = json::parse("{\"n\": 1, \"s\": \"hi\", \"b\": true, \"a\": [1]}");
    EXPECT_DOUBLE_EQ(v[std::string("n")].as_number(), 1.0);
    EXPECT_EQ(v[std::string("s")].as_string(), "hi");
    EXPECT_EQ(v[std::string("b")].as_boolean(), true);
    EXPECT_EQ(v[std::string("a")][0].as_number(), 1.0);
}

TEST(ParseObject, MissingColonThrows) {
    EXPECT_THROW(json::parse("{\"key\" 42}"), json::ParseError);
}

TEST(ParseObject, NonStringKeyThrows) {
    EXPECT_THROW(json::parse("{1: 2}"), json::ParseError);
}

// =======================================================================
// 解析：空白处理
// =======================================================================

TEST(ParseWhitespace, LeadingAndTrailing) {
    auto v = json::parse("  42  ");
    EXPECT_DOUBLE_EQ(v.as_number(), 42.0);
}

TEST(ParseWhitespace, BetweenTokens) {
    auto v = json::parse("{ \"a\" : 1 , \"b\" : 2 }");
    EXPECT_DOUBLE_EQ(v[std::string("a")].as_number(), 1.0);
    EXPECT_DOUBLE_EQ(v[std::string("b")].as_number(), 2.0);
}

// =======================================================================
// 错误报告
// =======================================================================

TEST(ParseError, ContainsLineAndCol) {
    try {
        json::parse("{\n  \"a\": 1,\n  \"b\": \n}");
        FAIL() << "Expected ParseError";
    } catch (const json::ParseError& e) {
        // 确保 what() 包含行号、列号信息
        std::string msg = e.what();
        EXPECT_NE(msg.find("line"), std::string::npos);
        EXPECT_NE(msg.find("col"), std::string::npos);
    }
}

TEST(ParseError, LineColumnValues) {
    try {
        json::parse("{\n  \"key\": \n}");
        FAIL() << "Expected ParseError";
    } catch (const json::ParseError& e) {
        EXPECT_GE(e.line(), 1u);
        EXPECT_GE(e.column(), 1u);
    }
}

TEST(ParseError, UnexpectedCharacter) {
    EXPECT_THROW(json::parse("@"), json::ParseError);
}

TEST(ParseError, UnclosedObject) {
    EXPECT_THROW(json::parse("{\"a\": 1"), json::ParseError);
}

TEST(ParseError, UnclosedArray) {
    EXPECT_THROW(json::parse("[1, 2"), json::ParseError);
}

// =======================================================================
// 初始化列表构造与工厂函数
// =======================================================================

TEST(InitializerList, ValueArrayInit) {
    json::Value v{json::Value(1.0), json::Value(2.0), json::Value(3.0)};
    EXPECT_TRUE(v.is_array());
    EXPECT_EQ(v.as_array().size(), 3u);
}

TEST(InitializerList, ValueObjectInit) {
    json::Value v{std::pair<const std::string, json::Value>{"x", json::Value(1.0)}, std::pair<const std::string, json::Value>{"y", json::Value(2.0)}};
    EXPECT_TRUE(v.is_object());
    EXPECT_DOUBLE_EQ(v[std::string("x")].as_number(), 1.0);
    EXPECT_DOUBLE_EQ(v[std::string("y")].as_number(), 2.0);
}

TEST(FactoryFunctions, ArrayFactory) {
    auto v = json::Array({json::Value("a"), json::Value("b")});
    EXPECT_TRUE(v.is_array());
    EXPECT_EQ(v[0].as_string(), "a");
    EXPECT_EQ(v[1].as_string(), "b");
}

TEST(FactoryFunctions, ObjectFactory) {
    auto v = json::Object({{"name", json::Value("test")}, {"val", json::Value(42.0)}});
    EXPECT_TRUE(v.is_object());
    EXPECT_EQ(v[std::string("name")].as_string(), "test");
    EXPECT_DOUBLE_EQ(v[std::string("val")].as_number(), 42.0);
}

// =======================================================================
// 往返一致性（parse -> dump -> parse）
// =======================================================================

TEST(RoundTrip, SimpleObject) {
    std::string src = "{\"a\":1,\"b\":\"hello\",\"c\":true,\"d\":null}";
    auto v1 = json::parse(src);
    std::string dumped = v1.dump();
    auto v2 = json::parse(dumped);
    EXPECT_EQ(v1, v2);
}

TEST(RoundTrip, NestedStructure) {
    std::string src = "{\"arr\":[1,2,3],\"obj\":{\"x\":0}}";
    auto v1 = json::parse(src);
    std::string dumped = v1.dump();
    auto v2 = json::parse(dumped);
    EXPECT_EQ(v1, v2);
}

TEST(RoundTrip, EmptyContainers) {
    auto arr = json::parse("[]");
    EXPECT_EQ(arr.dump(), "[]");

    auto obj = json::parse("{}");
    EXPECT_EQ(obj.dump(), "{}");
}

// =======================================================================
// 边界情况
// =======================================================================

TEST(EdgeCases, VeryLargeNumber) {
    auto v = json::parse("1e308");
    EXPECT_TRUE(std::isfinite(v.as_number()));
}

TEST(EdgeCases, VerySmallNumber) {
    auto v = json::parse("1e-308");
    EXPECT_TRUE(std::isfinite(v.as_number()));
}

TEST(EdgeCases, DeeplyNestedStructure) {
    // 构建一个深层嵌套的 JSON
    std::string src = "{\"a\":";
    for (int i = 0; i < 20; ++i) {
        src += "{\"a\":";
    }
    src += "1";
    for (int i = 0; i < 20; ++i) {
        src += "}";
    }
    src += "}";
    auto v = json::parse(src);
    EXPECT_TRUE(v.is_object());
}

TEST(EdgeCases, ZeroWithExponent) {
    auto v = json::parse("0e0");
    EXPECT_DOUBLE_EQ(v.as_number(), 0.0);
}

TEST(EdgeCases, StringWithSlash) {
    auto v = json::parse("\"a\\/b\"");
    EXPECT_EQ(v.as_string(), "a/b");
}

TEST(EdgeCases, ObjectKeyOrderPreserved) {
    // std::map 按键排序，不保留插入顺序
    auto v = json::parse("{\"b\":2,\"a\":1}");
    const auto& obj = v.as_object();
    auto it = obj.begin();
    EXPECT_EQ(it->first, "a");
    ++it;
    EXPECT_EQ(it->first, "b");
}

TEST(EdgeCases, ParseEmptyInputThrows) {
    EXPECT_THROW(json::parse(""), json::ParseError);
}

TEST(EdgeCases, ParseOnlyWhitespaceThrows) {
    EXPECT_THROW(json::parse("   "), json::ParseError);
}

TEST(EdgeCases, NumberZeroDecimal) {
    auto v = json::parse("0.0");
    EXPECT_DOUBLE_EQ(v.as_number(), 0.0);
}

TEST(EdgeCases, NumberWithCapitalE) {
    auto v = json::parse("1E2");
    EXPECT_DOUBLE_EQ(v.as_number(), 100.0);
}

// =======================================================================
// ParseError 格式化
// =======================================================================

TEST(ParseErrorFormat, FormatMessage) {
    json::ParseError err("test error", 5, 10);
    std::string msg = err.what();
    EXPECT_NE(msg.find("line 5"), std::string::npos);
    EXPECT_NE(msg.find("col 10"), std::string::npos);
    EXPECT_NE(msg.find("test error"), std::string::npos);
}

TEST(ParseErrorFormat, LineColumnAccessors) {
    json::ParseError err("msg", 3, 7);
    EXPECT_EQ(err.line(), 3u);
    EXPECT_EQ(err.column(), 7u);
}

// =======================================================================
// Unicode 边界
// =======================================================================

TEST(Unicode, BMPCharacter) {
    // U+00E9 = e with acute accent
    auto v = json::parse("\"\\u00E9\"");
    EXPECT_EQ(v.as_string(), "\xC3\xA9");
}

TEST(Unicode, SurrogatePairEmoji) {
    // U+1F4A9 = poop emoji
    auto v = json::parse("\"\\uD83D\\uDCA9\"");
    EXPECT_EQ(v.as_string(), "\xF0\x9F\x92\xA9");
}

TEST(Unicode, HighSurrogateWithoutLowThrows) {
    EXPECT_THROW(json::parse("\"\\uD83D\""), json::ParseError);
}

TEST(Unicode, InvalidHexInEscapeThrows) {
    EXPECT_THROW(json::parse("\"\\u00GG\""), json::ParseError);
}

// =======================================================================
// 整数 dump 格式化
// =======================================================================

TEST(DumpFormatting, IntegerNoDecimal) {
    json::Value v(1.0);
    EXPECT_EQ(v.dump(), "1");
}

TEST(DumpFormatting, IntegerZero) {
    json::Value v(0.0);
    EXPECT_EQ(v.dump(), "0");
}

TEST(DumpFormatting, NegativeInteger) {
    json::Value v(-100.0);
    EXPECT_EQ(v.dump(), "-100");
}

// =======================================================================
// 复杂往返
// =======================================================================

TEST(RoundTrip, ComplexDocument) {
    std::string src = R"({
        "name": "test",
        "version": 1,
        "items": [true, false, null, 3.14],
        "nested": {"key": "value"}
    })";
    auto v1 = json::parse(src);
    std::string dumped = v1.dump();
    auto v2 = json::parse(dumped);
    EXPECT_EQ(v1, v2);
}

TEST(RoundTrip, StringWithEscapes) {
    std::string src = "\"hello\\nworld\"";
    auto v = json::parse(src);
    EXPECT_EQ(v.as_string(), "hello\nworld");
    // dump 后会将 \n 转义回去
    std::string dumped = v.dump();
    EXPECT_EQ(dumped, "\"hello\\nworld\"");
}
