// SPDX-License-Identifier: MIT
// Copyright (c) 2026 MBition GmbH

#include <gtest/gtest.h>

#include "dltparser/dltparser.h"

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <vector>

namespace
{
    std::filesystem::path write_invalid_non_verbose_dlt()
    {
        const auto path = std::filesystem::temp_directory_path() /
                          ("dltparser-invalid-payload-" +
                           std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".DLT");
        const std::array<unsigned char, 30> message{
            'D',
            'L',
            'T',
            0x01,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            'T',
            'E',
            'S',
            'T',
            0x01,
            0x00,
            0x00,
            0x0E,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
            0x00,
        };
        std::ofstream out(path, std::ios::binary);
        out.write(reinterpret_cast<const char *>(message.data()), static_cast<std::streamsize>(message.size()));
        return path;
    }
}

TEST(ParserDiagnostics, ReportsInvalidNonVerbosePayloadLength)
{
    const auto path = write_invalid_non_verbose_dlt();
    std::vector<dltp::ParseDiagnostic> diagnostics;
    dltp::DLTFile<> parser(path.string(), dltp::DefaultSink{}, dltp::DefaultSink{},
                           [&diagnostics](const dltp::ParseDiagnostic &diagnostic)
                           {
                               diagnostics.push_back(diagnostic);
                           });

    ASSERT_TRUE(parser.is_read());
    ASSERT_TRUE(parser.parse());
    ASSERT_EQ(diagnostics.size(), 1U);
    EXPECT_EQ(diagnostics[0].code, dltp::ParseDiagnosticCode::INVALID_NON_VERBOSE_PAYLOAD_LENGTH);
    EXPECT_EQ(diagnostics[0].declared_length, 14);
    EXPECT_EQ(diagnostics[0].derived_payload_length, -4);

    std::filesystem::remove(path);
}
