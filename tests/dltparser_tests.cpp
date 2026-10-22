// SPDX-License-Identifier: MIT
// Copyright (c) 2026 MBition GmbH

#include <gtest/gtest.h>

#include "../include/dltparser/dltparser.h"

using namespace dltp;

//
// Constructor tests
//

// Test if file loading works if filepath exists and is a correct file
TEST(BasicParsing, FilepathExistsAndIsCorrectFile)
{
    // TODO: Add proper DLT test file
    DLTFile my_dlt_file("");

    EXPECT_EQ(my_dlt_file.is_read(), true);
}

// Test if file loading works if filepath does not exist
TEST(BasicParsing, FilepathDoesNotExist)
{
    DLTFile my_dlt_file("foo_not_exist.dlt");

    EXPECT_EQ(my_dlt_file.is_read(), false);
}

// Test if file loading works if filepath exists but is not a file but a directory
TEST(BasicParsing, FilepathExistsButIsNotFile)
{
    DLTFile my_dlt_file("CMakeFiles");

    EXPECT_EQ(my_dlt_file.is_read(), false);
}

// Test if file loading works if filepath exists but is a file with a wrong type
TEST(BasicParsing, FilepathExistsButIsWrongFileType)
{
    DLTFile my_dlt_file("CMakeCache.txt");

    EXPECT_EQ(my_dlt_file.is_read(), false);
}

TEST(BasicParsing, LoadedFileHasCorrectSize)
{
    // TODO: Add proper DLT test file
    DLTFile my_dlt_file("");

    ASSERT_EQ(my_dlt_file.is_read(), true);
    EXPECT_EQ(my_dlt_file.size(), static_cast<std::size_t>(0x1394u));
}

//
//
//
TEST(Header, ReportsWhetherStandardHeaderContainsEcuId)
{
    const char storage_buffer[] = {'N', 'E', 'W', ' '};
    StorageHeader storage_header{0, 0, storage_buffer, 0};
    const std::array<char, 4> default_ecuid{'N', 'U', 'L', 'L'};

    Header without_weid{0, storage_header, 0x21, 0, 4, 0, 0, default_ecuid};
    Header with_weid{0, storage_header, 0x25, 0, 8, 0, 0, default_ecuid};

    EXPECT_FALSE(without_weid.weid_present());
    EXPECT_TRUE(with_weid.weid_present());
}

TEST(StorageHeaderSearch, FindsMarkerAtBufferStart)
{
    const char buffer[] = {0x44, 0x4C, 0x54, 0x01, 'x'};

    const auto match = find_next_stohe(0, buffer, sizeof(buffer));

    ASSERT_TRUE(match.has_value());
    EXPECT_EQ(*match, 0);
}

TEST(StorageHeaderSearch, FindsMarkerAfterCurrentOffset)
{
    const char buffer[] = {'x', 'x', 0x44, 0x4C, 0x54, 0x01, 'x'};

    const auto match = find_next_stohe(1, buffer, sizeof(buffer));

    ASSERT_TRUE(match.has_value());
    EXPECT_EQ(*match, 2);
}

TEST(StorageHeaderSearch, DistinguishesNoMarkerFromOffsetZero)
{
    const char buffer[] = {'x', 'x', 'x', 'x'};

    const auto match = find_next_stohe(0, buffer, sizeof(buffer));

    EXPECT_FALSE(match.has_value());
}
