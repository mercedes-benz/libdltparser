// SPDX-License-Identifier: MIT
// Copyright (c) 2026 MBition GmbH

#include <gtest/gtest.h>
#include <array>

#ifdef __linux__
#include <cmath>
#endif

#include "../include/dltparser/utils.h"

// From here, tests for dltp::read()

TEST(ReadTest, ReadIntAtZeroOffset)
{
    int val = 42;
    std::vector<char> buffer(sizeof(int));
    std::memcpy(buffer.data(), &val, sizeof(int));

    std::streamsize offset = 0;
    int result = dltp::read<int>(buffer.data(), offset);

    EXPECT_EQ(result, 42);
    EXPECT_EQ(offset, sizeof(int));
}

TEST(ReadTest, ReadMultipleValuesInSequence)
{
    int a = 10;
    float b = 3.14f;
    char c = 'X';

    std::vector<char> buffer(sizeof(a) + sizeof(b) + sizeof(c));
    std::memcpy(buffer.data(), &a, sizeof(a));
    std::memcpy(buffer.data() + sizeof(a), &b, sizeof(b));
    std::memcpy(buffer.data() + sizeof(a) + sizeof(b), &c, sizeof(c));

    std::streamsize offset = 0;

    int resultA = dltp::read<int>(buffer.data(), offset);
    float resultB = dltp::read<float>(buffer.data(), offset);
    char resultC = dltp::read<char>(buffer.data(), offset);

    EXPECT_EQ(resultA, 10);
    EXPECT_FLOAT_EQ(resultB, 3.14f);
    EXPECT_EQ(resultC, 'X');
    EXPECT_EQ(offset, sizeof(a) + sizeof(b) + sizeof(c));
}

TEST(ReadTest, ReadAtNonZeroOffset)
{
    int a = 1;
    int b = 2;

    std::vector<char> buffer(sizeof(a) + sizeof(b));
    std::memcpy(buffer.data(), &a, sizeof(a));
    std::memcpy(buffer.data() + sizeof(a), &b, sizeof(b));

    std::streamsize offset = sizeof(a);
    int result = dltp::read<int>(buffer.data(), offset);

    EXPECT_EQ(result, 2);
    EXPECT_EQ(offset, sizeof(a) * 2);
}

TEST(ReadTest, ReadChar)
{
    char ch = 'Z';
    std::vector<char> buffer(1);
    buffer[0] = ch;

    std::streamsize offset = 0;
    char result = dltp::read<char>(buffer.data(), offset);

    EXPECT_EQ(result, 'Z');
    EXPECT_EQ(offset, 1);
}

// From here, tests for dltp::append_str_fixed()

TEST(AppendStrFixedTest, AppendWholeArray)
{
    std::vector<char> buf = {'a', 'b'};
    std::array<char, 4> arr = {'T', 'e', 's', 't'};

    dltp::append_str_fixed(buf, arr);
    std::string result(buf.begin(), buf.end());
    EXPECT_EQ(result, "abTest");
}

TEST(AppendStrFixedTest, AppendMultipleTimes)
{
    std::vector<char> buf;
    std::array<char, 3> arr = {'1', '2', '3'};

    dltp::append_str_fixed(buf, arr);
    dltp::append_str_fixed(buf, arr);
    std::string result(buf.begin(), buf.end());
    EXPECT_EQ(result, "123123");
}

TEST(AppendStrFixedTest, AppendEmptyArray)
{
    std::vector<char> buf = {'x', 'y'};
    std::array<char, 0> arr = {};

    dltp::append_str_fixed(buf, arr);
    std::string result(buf.begin(), buf.end());
    EXPECT_EQ(result, "xy"); // No change
}

TEST(AppendStrFixedTest, AppendSingleElementArray)
{
    std::vector<char> buf;
    std::array<char, 1> arr = {'Z'};

    dltp::append_str_fixed(buf, arr);
    std::string result(buf.begin(), buf.end());
    EXPECT_EQ(result, "Z");
}

// From here, tests for dltp::append_array_partly()

TEST(AppendArrayPartlyTest, AppendWholeArray)
{
    std::vector<char> buf = {'x', 'y'};
    std::array<char, 5> arr = {'H', 'e', 'l', 'l', 'o'};

    dltp::append_array_partly(buf, arr, arr.end());
    std::string result(buf.begin(), buf.end());
    EXPECT_EQ(result, "xyHello");
}

TEST(AppendArrayPartlyTest, AppendPartialArray)
{
    std::vector<char> buf;
    std::array<char, 5> arr = {'W', 'o', 'r', 'l', 'd'};

    dltp::append_array_partly(buf, arr, arr.begin() + 3);
    std::string result(buf.begin(), buf.end());
    EXPECT_EQ(result, "Wor");
}

TEST(AppendArrayPartlyTest, AppendEmptyRange)
{
    std::vector<char> buf = {'a', 'b'};
    std::array<char, 3> arr = {'X', 'Y', 'Z'};

    dltp::append_array_partly(buf, arr, arr.begin());
    std::string result(buf.begin(), buf.end());
    EXPECT_EQ(result, "ab"); // No change
}

TEST(AppendArrayPartlyTest, AppendMultipleTimes)
{
    std::vector<char> buf;
    std::array<char, 4> arr = {'t', 'e', 's', 't'};

    dltp::append_array_partly(buf, arr, arr.begin() + 2);
    dltp::append_array_partly(buf, arr, arr.end());
    std::string result(buf.begin(), buf.end());
    EXPECT_EQ(result, "tetest");
}

TEST(AppendArrayPartlyTest, EmptyArray)
{
    std::vector<char> buf;
    std::array<char, 0> arr = {};

    dltp::append_array_partly(buf, arr, arr.end());
    EXPECT_TRUE(buf.empty());
}

TEST(AppendArrayPartlyTest, SingleElementArray)
{
    std::vector<char> buf;
    std::array<char, 1> arr = {'Z'};

    dltp::append_array_partly(buf, arr, arr.end());
    std::string result(buf.begin(), buf.end());
    EXPECT_EQ(result, "Z");
}

// From here, tests for dltp::append_number()

template <typename T>
std::string append_num_to_str(T num)
{
    std::vector<char> buf;
    dltp::append_number(buf, num);
    return std::string(buf.begin(), buf.end());
}

TEST(AppendNumberTest, IntegralValues)
{
    EXPECT_EQ(append_num_to_str<int>(0), "0");
    EXPECT_EQ(append_num_to_str<int>(12345), "12345");
    EXPECT_EQ(append_num_to_str<int>(-9876), "-9876");
    EXPECT_EQ(append_num_to_str<unsigned int>(4294967295u), "4294967295");
}

TEST(AppendNumberTest, FloatingPointValues)
{
    // Exact string output can vary, so we parse back to float/double to check close equality
    auto check_float = [](float val)
    {
        std::string s = append_num_to_str(val);
        float parsed = std::stof(s);
        EXPECT_FLOAT_EQ(parsed, val);
    };

    auto check_double = [](double val)
    {
        std::string s = append_num_to_str(val);
        double parsed = std::stod(s);
        EXPECT_DOUBLE_EQ(parsed, val);
    };

    check_float(0.0f);
    check_float(-0.0f);
    check_float(3.14159f);
    check_float(-12345.6789f);
    check_float(1.23e10f);

    check_double(0.0);
    check_double(-0.0);
    check_double(3.141592653589793);
    check_double(-12345.6789012345);
    check_double(1.23e100);
}

TEST(AppendNumberTest, FloatingPointSpecialValues)
{
    // std::to_chars on floats does not handle NaN/Inf in all implementations,
    // so we check output length or known behavior loosely.

    std::vector<char> buf;

    float nan = std::nanf("");
    buf.clear();
    dltp::append_number(buf, nan);
    std::string s_nan(buf.begin(), buf.end());
    // May output "nan" or empty or something else, so just check non-empty or "nan"
    EXPECT_TRUE(s_nan == "nan" || s_nan == "NaN" || s_nan.empty());

    float inf = std::numeric_limits<float>::infinity();
    buf.clear();
    dltp::append_number(buf, inf);
    std::string s_inf(buf.begin(), buf.end());
    // May output "inf" or "INF" or empty - loose check
    EXPECT_TRUE(s_inf == "inf" || s_inf == "INF" || s_inf.empty());

    float ninf = -std::numeric_limits<float>::infinity();
    buf.clear();
    dltp::append_number(buf, ninf);
    std::string s_ninf(buf.begin(), buf.end());
    EXPECT_TRUE(s_ninf == "-inf" || s_ninf == "-INF" || s_ninf.empty());
}

// From here, tests for dltp::append_uint8()

TEST(AppendUint8Test, HandlesZero)
{
    std::vector<char> buf;
    dltp::append_uint8(buf, 0);
    EXPECT_EQ(std::string(buf.begin(), buf.end()), "0");
}

TEST(AppendUint8Test, HandlesSingleDigit)
{
    std::vector<char> buf;
    dltp::append_uint8(buf, 7);
    EXPECT_EQ(std::string(buf.begin(), buf.end()), "7");
}

TEST(AppendUint8Test, HandlesTwoDigitLow)
{
    std::vector<char> buf;
    dltp::append_uint8(buf, 10);
    EXPECT_EQ(std::string(buf.begin(), buf.end()), "10");
}

TEST(AppendUint8Test, HandlesTwoDigitHigh)
{
    std::vector<char> buf;
    dltp::append_uint8(buf, 99);
    EXPECT_EQ(std::string(buf.begin(), buf.end()), "99");
}

TEST(AppendUint8Test, HandlesThreeDigitLow)
{
    std::vector<char> buf;
    dltp::append_uint8(buf, 100);
    EXPECT_EQ(std::string(buf.begin(), buf.end()), "100");
}

TEST(AppendUint8Test, HandlesMaxValue)
{
    std::vector<char> buf;
    dltp::append_uint8(buf, 255);
    EXPECT_EQ(std::string(buf.begin(), buf.end()), "255");
}

// From here, tests for dltp::append_int8()

TEST(AppendInt8Test, HandlesZero)
{
    std::vector<char> buf;
    dltp::append_int8(buf, 0);
    EXPECT_EQ(std::string(buf.begin(), buf.end()), "0");
}

TEST(AppendInt8Test, HandlesSingleDigitPositive)
{
    std::vector<char> buf;
    dltp::append_int8(buf, 7);
    EXPECT_EQ(std::string(buf.begin(), buf.end()), "7");
}

TEST(AppendInt8Test, HandlesTwoDigitPositive)
{
    std::vector<char> buf;
    dltp::append_int8(buf, 42);
    EXPECT_EQ(std::string(buf.begin(), buf.end()), "42");
}

TEST(AppendInt8Test, HandlesMaxPositive)
{
    std::vector<char> buf;
    dltp::append_int8(buf, 127);
    EXPECT_EQ(std::string(buf.begin(), buf.end()), "127");
}

TEST(AppendInt8Test, HandlesSingleDigitNegative)
{
    std::vector<char> buf;
    dltp::append_int8(buf, -3);
    EXPECT_EQ(std::string(buf.begin(), buf.end()), "-3");
}

TEST(AppendInt8Test, HandlesTwoDigitNegative)
{
    std::vector<char> buf;
    dltp::append_int8(buf, -85);
    EXPECT_EQ(std::string(buf.begin(), buf.end()), "-85");
}

TEST(AppendInt8Test, HandlesMinNegative)
{
    std::vector<char> buf;
    dltp::append_int8(buf, -128);
    EXPECT_EQ(std::string(buf.begin(), buf.end()), "-128");
}

// From here, tests for dltp::byteswap()

TEST(ByteswapTest, ByteswapUint16)
{
    uint16_t val = 0x1234;
    uint16_t expected = 0x3412;
    EXPECT_EQ(dltp::byteswap(val), expected);

    val = 0x0000;
    expected = 0x0000;
    EXPECT_EQ(dltp::byteswap(val), expected);

    val = 0xFFFF;
    expected = 0xFFFF;
    EXPECT_EQ(dltp::byteswap(val), expected);
}

TEST(ByteswapTest, ByteswapInt16)
{
    int16_t val = 0x1234;
    int16_t expected = 0x3412;
    EXPECT_EQ(dltp::byteswap(val), expected);

    val = -1; // 0xFFFF in 16-bit
    expected = -1;
    EXPECT_EQ(dltp::byteswap(val), expected);
}

TEST(ByteswapTest, ByteswapUint32)
{
    uint32_t val = 0x12345678;
    uint32_t expected = 0x78563412;
    EXPECT_EQ(dltp::byteswap(val), expected);

    val = 0;
    expected = 0;
    EXPECT_EQ(dltp::byteswap(val), expected);

    val = 0xFFFFFFFF;
    expected = 0xFFFFFFFF;
    EXPECT_EQ(dltp::byteswap(val), expected);
}

TEST(ByteswapTest, ByteswapInt32)
{
    int32_t val = 0x12345678;
    int32_t expected = 0x78563412;
    EXPECT_EQ(dltp::byteswap(val), expected);

    val = -1; // 0xFFFFFFFF
    expected = -1;
    EXPECT_EQ(dltp::byteswap(val), expected);
}

TEST(ByteswapTest, ByteswapUint64)
{
    uint64_t val = 0x0123456789ABCDEFULL;
    uint64_t expected = 0xEFCDAB8967452301ULL;
    EXPECT_EQ(dltp::byteswap(val), expected);

    val = 0;
    expected = 0;
    EXPECT_EQ(dltp::byteswap(val), expected);

    val = 0xFFFFFFFFFFFFFFFFULL;
    expected = 0xFFFFFFFFFFFFFFFFULL;
    EXPECT_EQ(dltp::byteswap(val), expected);
}

TEST(ByteswapTest, ByteswapInt64)
{
    int64_t val = 0x0123456789ABCDEFLL;
    int64_t expected = 0xEFCDAB8967452301LL;
    EXPECT_EQ(dltp::byteswap(val), expected);

    val = -1; // 0xFFFFFFFFFFFFFFFF
    expected = -1;
    EXPECT_EQ(dltp::byteswap(val), expected);
}

// From here, tests for dltp::appends_str_hex()

TEST(AppendsStrHexTest, HandlesEmptyInput)
{
    std::vector<char> buf;
    dltp::appends_str_hex(buf, "", 0);
    EXPECT_TRUE(buf.empty());
}

TEST(AppendsStrHexTest, ConvertsSingleByte)
{
    std::vector<char> buf;
    char data[] = {static_cast<char>(0xAB)};
    dltp::appends_str_hex(buf, data, 1);
    std::string expected = "AB ";
    EXPECT_EQ(std::string(buf.begin(), buf.end()), expected);
}

TEST(AppendsStrHexTest, ConvertsMultipleBytes)
{
    std::vector<char> buf;
    char data[] = {static_cast<char>(0x00), static_cast<char>(0xFF), static_cast<char>(0x1A), static_cast<char>(0xC3)};
    dltp::appends_str_hex(buf, data, 4);
    std::string expected = "00 FF 1A C3 ";
    EXPECT_EQ(std::string(buf.begin(), buf.end()), expected);
}

TEST(AppendsStrHexTest, AppendsToExistingBuffer)
{
    std::vector<char> buf = {'X', 'Y'};
    char data[] = {0x0F};
    dltp::appends_str_hex(buf, data, 1);
    std::string expected = "XY0F ";
    EXPECT_EQ(std::string(buf.begin(), buf.end()), expected);
}

TEST(AppendsStrHexTest, HandlesZeroByte)
{
    std::vector<char> buf;
    char data[] = {0x00};
    dltp::appends_str_hex(buf, data, 1);
    std::string expected = "00 ";
    EXPECT_EQ(std::string(buf.begin(), buf.end()), expected);
}

TEST(AppendsStrHexTest, HandlesMaxByte)
{
    std::vector<char> buf;
    char data[] = {static_cast<char>(0xFF)};
    dltp::appends_str_hex(buf, data, 1);
    std::string expected = "FF ";
    EXPECT_EQ(std::string(buf.begin(), buf.end()), expected);
}

// From here, tests for dltp::contains() function not using regex

TEST(ContainsTest, ReturnsTrueWhenSubstringPresent)
{
    EXPECT_TRUE(dltp::contains("hello world", "hello")); // substring at start
    EXPECT_TRUE(dltp::contains("hello world", "world")); // substring at end
    EXPECT_TRUE(dltp::contains("hello world", "lo wo")); // substring in middle
    EXPECT_TRUE(dltp::contains("hello world", " "));     // substring is a space
}

TEST(ContainsTest, ReturnsFalseWhenSubstringAbsent)
{
    EXPECT_FALSE(dltp::contains("hello world", "worlds")); // substring not present
    EXPECT_FALSE(dltp::contains("hello world", "planet")); // substring completely different
}

TEST(ContainsTest, HandlesEmptyStrings)
{
    EXPECT_TRUE(dltp::contains("", ""));    // empty substring in empty string -> true
    EXPECT_TRUE(dltp::contains("abc", "")); // empty substring in non-empty string -> true
    EXPECT_FALSE(dltp::contains("", "a"));  // non-empty substring in empty string -> false
}

TEST(ContainsTest, SubstringEqualsString)
{
    EXPECT_TRUE(dltp::contains("test", "test")); // substring same as string
}

TEST(ContainsTest, CaseSensitiveSearch)
{
    EXPECT_FALSE(dltp::contains("Hello", "hello")); // case sensitive check, should be false
}

// From here, tests for dltp::contains() function with regex

TEST(ContainsRegexTest, SearchReturnsTrueIfPartialMatch)
{
    std::regex re("world");
    EXPECT_TRUE((dltp::contains<true>("hello world", re))); // substring match
    EXPECT_FALSE((dltp::contains<true>("hello", re)));      // no match
}

TEST(ContainsRegexTest, MatchReturnsTrueOnlyIfFullStringMatches)
{
    std::regex re("^hello world$");
    EXPECT_TRUE((dltp::contains<false>("hello world", re)));   // exact match
    EXPECT_FALSE((dltp::contains<false>("hello world!", re))); // partial mismatch
    EXPECT_FALSE((dltp::contains<false>("hello", re)));        // no match
}

TEST(ContainsRegexTest, EmptyStringAndRegex)
{
    std::regex empty_re("");
    EXPECT_TRUE((dltp::contains<true>("", empty_re)));  // search empty regex in empty string (matches)
    EXPECT_TRUE((dltp::contains<false>("", empty_re))); // match empty regex to empty string (matches)
}

TEST(ContainsRegexTest, RegexMatchesEmptySubstring)
{
    std::regex re("a*");                              // matches zero or more 'a's, so empty substring matches everywhere
    EXPECT_TRUE((dltp::contains<true>("bbb", re)));   // search finds empty match at start
    EXPECT_FALSE((dltp::contains<false>("bbb", re))); // full string "bbb" does NOT match "a*" regex
}

TEST(ContainsRegexTest, RegexStartEndAnchors)
{
    std::regex start_re("^hello");
    std::regex end_re("world$");

    EXPECT_TRUE((dltp::contains<true>("hello world", start_re)));   // search finds start match
    EXPECT_FALSE((dltp::contains<false>("hello world", start_re))); // full string does not exactly match "^hello"

    EXPECT_TRUE((dltp::contains<true>("hello world", end_re)));   // search finds end match
    EXPECT_FALSE((dltp::contains<false>("hello world", end_re))); // full string does not exactly match "world$"
}