// SPDX-License-Identifier: MIT
// Copyright (c) 2026 MBition GmbH

#include <gtest/gtest.h>

#include "../include/dltparser/dltsettings.h"

TEST(SettingsTest, DefaultConstructor)
{
    dltp::Settings s;

    EXPECT_EQ(s.get_msg_buffer_reserve_count(), 0x1E8480);
    EXPECT_FALSE(s.get_with_raw_in_hex());
    EXPECT_FALSE(s.with_substr());
    EXPECT_FALSE(s.with_regex_search());
    EXPECT_FALSE(s.with_regex_match());
    EXPECT_TRUE(s.get_substr_strs().empty());
    EXPECT_TRUE(s.get_search_regex_strs().empty());
    EXPECT_TRUE(s.get_match_regex_strs().empty());
}

TEST(SettingsTest, CustomInitialization)
{
    std::vector<std::string> substrs = {"foo", "bar"};
    std::vector<std::pair<std::string, dltp::Settings::RePattern>> regexes = {
        {"a.*", dltp::Settings::REGEX_SEARCH},
        {"b.*", dltp::Settings::REGEX_MATCH}};

    dltp::Settings s(123456, true, true, substrs, regexes);

    EXPECT_EQ(s.get_msg_buffer_reserve_count(), 123456);
    EXPECT_TRUE(s.get_with_raw_in_hex());
    EXPECT_TRUE(s.with_substr());
    EXPECT_TRUE(s.with_regex_search());
    EXPECT_TRUE(s.with_regex_match());
    EXPECT_EQ(s.get_substr_strs(), substrs);

    auto search = s.get_search_regex_strs();
    auto match = s.get_match_regex_strs();

    EXPECT_EQ(search, std::vector<std::string>({"a.*"}));
    EXPECT_EQ(match, std::vector<std::string>({"b.*"}));
}

TEST(SettingsTest, OnlySubstrs)
{
    dltp::Settings s(100, false, true, {"needle"});

    EXPECT_TRUE(s.with_substr());
    EXPECT_FALSE(s.with_regex_search());
    EXPECT_FALSE(s.with_regex_match());
}

TEST(SettingsTest, OnlySearchRegex)
{
    dltp::Settings s(100, false, true, {}, {{"abc.*", dltp::Settings::REGEX_SEARCH}});

    EXPECT_FALSE(s.with_substr());
    EXPECT_TRUE(s.with_regex_search());
    EXPECT_FALSE(s.with_regex_match());
}

TEST(SettingsTest, CopyConstructor)
{
    dltp::Settings original(100, true, true, {"x"}, {{"y", dltp::Settings::REGEX_MATCH}});
    dltp::Settings copy = original;

    EXPECT_EQ(copy.get_msg_buffer_reserve_count(), 100);
    EXPECT_TRUE(copy.get_with_raw_in_hex());
    EXPECT_TRUE(copy.with_substr());
    EXPECT_TRUE(copy.with_regex_match());
}

TEST(SettingsTest, MoveConstructor)
{
    dltp::Settings original(200, false, true, {"a"}, {{"b", dltp::Settings::REGEX_SEARCH}});
    dltp::Settings moved = std::move(original);

    EXPECT_EQ(moved.get_msg_buffer_reserve_count(), 200);
    EXPECT_TRUE(moved.with_substr());
    EXPECT_TRUE(moved.with_regex_search());
}

TEST(SettingsTest, ThrowsOnInvalidRegex)
{
    std::vector<std::pair<std::string, dltp::Settings::RePattern>> regexes = {
        {"[invalid", dltp::Settings::REGEX_SEARCH} // Missing closing bracket
    };

    EXPECT_THROW({ dltp::Settings s(100, false, true, {}, regexes); }, std::regex_error);
}