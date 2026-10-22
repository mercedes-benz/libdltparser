// SPDX-License-Identifier: MIT
// Copyright (c) 2026 MBition GmbH

#ifndef DLTPARSER_UTILS_H
#define DLTPARSER_UTILS_H

#include <vector>
#include <iostream>
#include <charconv>
#include <cstring>
#include <cstdio>
#include <regex>

// MacOS availability check
#if defined(__APPLE__) && defined(__MAC_OS_X_VERSION_MIN_REQUIRED) && __MAC_OS_X_VERSION_MIN_REQUIRED < 130300
#define DISABLE_TO_CHARS_FLOAT 1
#elif defined(_LIBCPP_VERSION) && _LIBCPP_VERSION < 15000
    // For older libc++ versions without float to_chars
#define DISABLE_TO_CHARS_FLOAT 1
#else
#define DISABLE_TO_CHARS_FLOAT 0
#endif

namespace dltp
{

    /// Reads a value from a memory mapped file starting at the ptr and advances the offset by the size of T
    /// \tparam T The type of the value which is being read
    /// \param buffer The buffer which holds the value
    /// \param offset The beginning of the value which is being red
    /// \return The read value
    template <typename T>
    [[nodiscard]] T read(const char *buffer, std::streamsize &offset) noexcept
    {
        T value;
        std::memcpy(&value, buffer + offset, sizeof(T));
        offset += sizeof(T);
        return value;
    }

    /// Appends a fixed size char array to a given vector of chars
    /// \tparam T The type of the array which has to be char
    /// \tparam U The size of the appended string
    /// \param str_buf The char vector to append to
    /// \param arr The array which holds the string
    template <typename T, std::size_t U, typename = std::enable_if_t<std::is_same_v<T, char>>>
    void append_str_fixed(std::vector<char> &str_buf, const std::array<T, U> &arr) noexcept
    {
        str_buf.insert(str_buf.end(), arr.begin(), arr.end());
    }

    /// Appends a parts of a fixed size char array to a given vector of chars
    /// \tparam T The type of the array which has to be char
    /// \tparam U The size of the appended string
    /// \param str_buf The char vector to append to
    /// \param arr The array which holds the string
    /// \param end_it The end iterator indicating the last char of the fixed size char array which shall be appended
    template <typename T, std::size_t U, typename = std::enable_if_t<std::is_same_v<T, char>>>
    void append_array_partly(std::vector<char> &str_buf, const std::array<T, U> &arr, typename std::array<T, U>::const_iterator end_it) noexcept
    {
        str_buf.insert(str_buf.end(), arr.begin(), end_it);
    }

    /// Appends the string representation of a number to the given vector of chars
    /// \tparam T The type of the number
    /// \param str_buf The vector of chars to append to
    /// \param num The actual number to append
    template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    void append_number(std::vector<char> &str_buf, T num) noexcept
    {
        char num_buf[64];
        if constexpr (std::is_integral_v<T>)
        {
            auto [ptr, ec] = std::to_chars(num_buf, num_buf + 64, num);
            str_buf.insert(str_buf.end(), num_buf, ptr);
        }
        else if constexpr (std::is_floating_point_v<T>)
        {
#if DISABLE_TO_CHARS_FLOAT
            int len = std::snprintf(num_buf, sizeof(num_buf), "%g", num);
            if (len > 0 && len < static_cast<int>(sizeof(num_buf)))
            {
                str_buf.insert(str_buf.end(), num_buf, num_buf + len);
            }
#else
            auto [ptr, ec] = std::to_chars(num_buf, num_buf + 64, num, std::chars_format::general);
            str_buf.insert(str_buf.end(), num_buf, ptr);
        }
#endif
        }

        /// @brief Converts an uint8_t to a string and adds it to the buffer
        /// @param str_buf The string buffer to append to
        /// @param val The number to convert adn append
        inline void append_uint8(std::vector<char> & str_buf, uint8_t val)
        {
            if (val >= 100)
            {
                str_buf.push_back('0' + val / 100);
                val %= 100;
                str_buf.push_back('0' + val / 10);
                str_buf.push_back('0' + val % 10);
            }
            else if (val >= 10)
            {
                str_buf.push_back('0' + val / 10);
                str_buf.push_back('0' + val % 10);
            }
            else
            {
                str_buf.push_back('0' + val);
            }
        }

        /// @brief Converts an int8_t to a string and adds it to the buffer
        /// @param str_buf The string buffer to append to
        /// @param val The number to convert adn append
        inline void append_int8(std::vector<char> & str_buf, int8_t val)
        {
            unsigned char uval;

            if (val < 0)
            {
                str_buf.push_back('-');
                uval = static_cast<unsigned char>(-val);
            }
            else
            {
                uval = static_cast<unsigned char>(val);
            }

            if (uval >= 100)
            {
                str_buf.push_back('0' + uval / 100);
                uval %= 100;
                str_buf.push_back('0' + uval / 10);
                str_buf.push_back('0' + uval % 10);
            }
            else if (uval >= 10)
            {
                str_buf.push_back('0' + uval / 10);
                str_buf.push_back('0' + uval % 10);
            }
            else
            {
                str_buf.push_back('0' + uval);
            }
        }

        /// @brief Swaps the bytes for integral types
        /// @tparam T The intergral type
        /// @tparam
        /// @param value The actual number
        /// @return The number with the bytes swapped
        template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
        [[nodiscard]] T byteswap(T value)
        {
            if constexpr (sizeof(T) == 2)
            {
                return static_cast<T>((value >> 8) | (value << 8));
            }
            else if constexpr (sizeof(T) == 4)
            {
                return static_cast<T>(((value >> 24) & 0xFF) |
                                      ((value >> 8) & 0xFF00) |
                                      ((value << 8) & 0xFF0000) |
                                      ((value << 24) & 0xFF000000));
            }
            else if constexpr (sizeof(T) == 8)
            {
                return static_cast<T>(((value >> 56) & 0xFF) |
                                      ((value >> 40) & 0xFF00) |
                                      ((value >> 24) & 0xFF0000) |
                                      ((value >> 8) & 0xFF000000) |
                                      ((value << 8) & 0xFF00000000) |
                                      ((value << 24) & 0xFF0000000000) |
                                      ((value << 40) & 0xFF000000000000) |
                                      ((value << 56) & 0xFF00000000000000));
            }
            else
            {
                static_assert(sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8, "Unsupported integer size for byteswap");
            }
        }

        /// @brief Appends the given byte array as hex-encoded string to the given buffer
        /// @param str_buf The buffer to append the hex-encoded string
        /// @param data The byte data
        /// @param length The length of the byte data
        inline void appends_str_hex(std::vector<char> & str_buf, const char *const data, std::streamsize length)
        {
            static constexpr char hex_digits[] = "0123456789ABCDEF";

            for (std::streamsize i = 0; i < length; ++i)
            {
                auto const byte = static_cast<unsigned char>(data[i]);
                str_buf.emplace_back(hex_digits[(byte >> 4) & 0xF]);
                str_buf.emplace_back(hex_digits[byte & 0xF]);
                str_buf.emplace_back(' ');
            }
        }

        /// @brief Simple substring search within the given string
        /// @param str The string to check
        /// @param substr The substring
        /// @return True if the substring is present within the string, otherwise false
        [[nodiscard]] inline bool contains(const std::string_view &str, const std::string_view &substr) noexcept
        {
            return str.find(substr) != std::string_view::npos;
        }

        /// @brief Use regex search/match on the given string
        /// @tparam use_search If true, std::regex_search is used, otherwise std::regex_match is used
        /// @param str The string to search/match against
        /// @param re The regex
        /// @return The result of either std::regex_search or std::regex_match
        template <bool use_search>
        [[nodiscard]] bool contains(const std::string_view &str, const std::regex &re) noexcept
        {
            if constexpr (use_search)
            {
                return std::regex_search(str.begin(), str.end(), re);
            }
            else
            {
                return std::regex_match(str.begin(), str.end(), re);
            }
        }

    } // namespace dltp

#endif // DLTPARSER_UTILS_H
