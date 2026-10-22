// SPDX-License-Identifier: MIT
// Copyright (c) 2026 MBition GmbH

#ifndef DLTPARSER_DLTSETTINGS_H
#define DLTPARSER_DLTSETTINGS_H

#include <regex>

namespace dltp
{

    /// @brief The Settings objects defines what you are going to search within the Payload during the parsing process.
    /// You can also specify the amount of messages you are expecting and if you want to get a hexadecimal string from raw byte messages
    class Settings
    {

        // Interface
    public:
        /// @brief Regex pattern which has to be passed together with a regex string
        enum RePattern
        {
            REGEX_SEARCH,
            REGEX_MATCH
        };

        ~Settings() noexcept = default;

        /// @brief Constructor to create the Settings for parsing
        /// @param msg_buf_res_count The amount of messages to be expected; the internal buffer will be reserved accordingly
        /// @param with_raw_in_hex If true, raw byte payloads will be hex-encoded
        /// @param save_msgs If true, messages will be saved either via callback or within the DLTFile object, otherwise not
        /// @param substrs Substrings used for plain text search within payloads
        /// @param re_strs Regular expressions to be used for payload filtering
        /// @throws std::regex_error if the regex could not be created or std::vector related exceptions, e.g. std::bad_alloc
        explicit Settings(
            const std::size_t msg_buf_res_count = 0x00000000001E8480,
            const bool with_raw_in_hex = false,
            const bool save_msgs = true,
            const std::vector<std::string> &substrs = {},
            const std::vector<std::pair<std::string, RePattern>> &re_strs = {})
            : msg_buffer_reserve_count(msg_buf_res_count),
              with_raw_in_hex(with_raw_in_hex),
              save_msgs(save_msgs),
              substr_strs(substrs)
        {
            for (const auto &[fst, snd] : re_strs)
            {
                if (snd == REGEX_SEARCH)
                {
                    search_regex.emplace_back(fst, std::regex{fst});
                }
                else if (snd == REGEX_MATCH)
                {
                    match_regex.emplace_back(fst, std::regex{fst});
                }
            }
        }

        // Copy Constructor
        Settings(const Settings &) noexcept = default;
        Settings &operator=(const Settings &) noexcept = default;

        // Move constructor
        Settings(Settings &&) noexcept = default;
        Settings &operator=(Settings &&) noexcept = default;

        /// @return True if a substring search string was passed during object creation, otherwise false
        [[nodiscard]] bool with_substr() const noexcept;

        /// @return True if a regex string with search pattern was passed during object creation, otherwise false
        [[nodiscard]] bool with_regex_search() const noexcept;

        /// @return True if a regex string with match pattern was passed during object creation, otherwise false
        [[nodiscard]] bool with_regex_match() const noexcept;

        /// @return True if with_raw_in_hex was passed as true during object creation, otherwise false
        [[nodiscard]] bool get_with_raw_in_hex() const noexcept;

        /// @return The amount which was passed for msg_buffer_reserve_count during object creation, otherwise 2 million
        [[nodiscard]] std::size_t get_msg_buffer_reserve_count() const noexcept;

        /// @return The substring search strings which were passed during object creation, otherwise an empty vector
        [[nodiscard]] const std::vector<std::string> &get_substr_strs() const noexcept;

        /// @return The regex strings with search pattern which were passed during object creation, otherwise an empty vector
        [[nodiscard]] std::vector<std::string> get_search_regex_strs() const noexcept;

        /// @return The regex strings with match pattern which were passed during object creation, otherwise an empty vector
        [[nodiscard]] std::vector<std::string> get_match_regex_strs() const noexcept;

        // Public member
    public:
        std::size_t msg_buffer_reserve_count;
        bool with_raw_in_hex = false;
        bool save_msgs = true;
        std::vector<std::string> substr_strs;
        std::vector<std::pair<std::string, std::regex>> search_regex;
        std::vector<std::pair<std::string, std::regex>> match_regex;
    };

    inline bool Settings::with_substr() const noexcept { return !substr_strs.empty(); }
    inline bool Settings::with_regex_search() const noexcept { return !search_regex.empty(); }
    inline bool Settings::with_regex_match() const noexcept { return !match_regex.empty(); }

    inline bool Settings::get_with_raw_in_hex() const noexcept { return with_raw_in_hex; }
    inline std::size_t Settings::get_msg_buffer_reserve_count() const noexcept { return msg_buffer_reserve_count; }
    inline const std::vector<std::string> &Settings::get_substr_strs() const noexcept { return substr_strs; }
    inline std::vector<std::string> Settings::get_search_regex_strs() const noexcept
    {
        std::vector<std::string> tmp;
        tmp.reserve(search_regex.size());

        for (const auto &[first, second] : search_regex)
        {
            tmp.push_back(first);
        }

        return tmp;
    }

    inline std::vector<std::string> Settings::get_match_regex_strs() const noexcept
    {
        std::vector<std::string> tmp;
        tmp.reserve(match_regex.size());

        for (const auto &[first, second] : match_regex)
        {
            tmp.push_back(first);
        }

        return tmp;
    }

} // namespace dltp

#endif // DLTPARSER_DLTSETTINGS_H