// SPDX-License-Identifier: MIT
// Copyright (c) 2026 MBition GmbH

#ifndef DLTPARSER_DLTPARSER_H
#define DLTPARSER_DLTPARSER_H

#ifdef WIN32
#include <windows.h>
#endif

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>

#ifdef __linux__
#include <sys/stat.h>
#include <immintrin.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#include "dltsettings.h"
#include "dltheader.h"
#include "dltpayload.h"
#include "utils.h"

#include "../include/dltparser/file_loader.h"

#ifdef DEBUG
#define DEBUG_MSG(str)                 \
    do                                 \
    {                                  \
        std::cout << str << std::endl; \
    } while (false)
#else
#define DEBUG_MSG(str) \
    do                 \
    {                  \
    } while (false)
#endif

namespace fs = std::filesystem;

namespace dltp
{

    struct DefaultSink
    {
    };

    enum class ParseDiagnosticCode
    {
        INCOMPLETE_BUFFER_FRAGMENT,
        CORRUPT_CARRY_OVER,
        INVALID_NON_VERBOSE_PAYLOAD_LENGTH,
    };

    struct ParseDiagnostic
    {
        ParseDiagnosticCode code;
        std::streamsize offset;
        uint16_t declared_length;
        std::streamsize derived_payload_length;
    };

    using DiagnosticSink = std::function<void(const ParseDiagnostic &)>;

    [[nodiscard]] constexpr std::string_view diagnostic_code_name(ParseDiagnosticCode code) noexcept
    {
        switch (code)
        {
        case ParseDiagnosticCode::INCOMPLETE_BUFFER_FRAGMENT:
            return "incomplete_buffer_fragment";
        case ParseDiagnosticCode::CORRUPT_CARRY_OVER:
            return "corrupt_carry_over";
        case ParseDiagnosticCode::INVALID_NON_VERBOSE_PAYLOAD_LENGTH:
            return "invalid_non_verbose_payload_length";
        }
        return "unknown";
    }

    /// @brief The DLTFile is the main object you are using to parse an AUTOSAR DLT file.
    /// It first loads the entire file into memory and then parses based on the given Settings.
    /// After parsing, you can access all messages or hits based on your passed substring or regex strings within your Settings.
    template <typename AddSink = DefaultSink, typename RemoveSink = DefaultSink>
    class DLTFile
    {
    public:
        /// @brief The simple default constructor to parse a DLT file without any substring or regex search.
        /// @param filepath The absolute filepath to the DLT file
        /// @param add_sink Optional: The sink callback which is called for each parsed message. If this sink is given, the remove_sink needs to be given too.
        /// @param remove_sink Optional: The sink callback which is called for each message when the previous message was broken and should be removed. If this sink is given, the add_sink needs to be given too.
        /// @param diagnostic_sink Optional: Receives recoverable parser diagnostics. Exceptions from this callback are suppressed.
        explicit DLTFile(const std::string &filepath, AddSink add_sink = AddSink{}, RemoveSink remove_sink = RemoveSink{}, DiagnosticSink diagnostic_sink = {}) noexcept;

        /// @brief The standard constructor to parse a DLT file and search for substrings or regex within the message payloads.
        /// @param filepath The absolute filepath to the DLT file
        /// @param settings The settings object in which substrings, regex and other options are given for parsing
        /// @param add_sink Optional: The sink callback which is called for each parsed message. If this sink is given, the remove_sink needs to be given too.
        /// @param remove_sink Optional: The sink callback which is called for each message when the previous message was broken and should be removed. If this sink is given, the add_sink needs to be given too.
        /// @param diagnostic_sink Optional: Receives recoverable parser diagnostics. Exceptions from this callback are suppressed.
        DLTFile(const std::string &filepath, const Settings &settings, AddSink add_sink = AddSink{}, RemoveSink remove_sink = RemoveSink{}, DiagnosticSink diagnostic_sink = {}) noexcept;

        /// @brief Starts the parsing of the DLT file
        /// @return True if parsing finished successfully, otherwise false
        bool parse() noexcept;

        /// @brief Saves all messages in a text file as human readable string
        /// @param filepath The absolute filepath
        /// @return True if saving was successful, otherwise false
        bool save_messages(const std::string &filepath) const noexcept;

        /// @brief Saves all hits of the substring or regex search in a text file as human readable string
        /// @param filepath The absolute filepath
        /// @return True if saving was successful, otherwise false
        bool save_hits(const std::string &filepath) const noexcept;

        /// @brief Get all messages of the parsed DLT file as a pair of Header and Payload
        /// @return All messages as Header, Payload pair
        [[nodiscard]] const std::vector<std::pair<Header, Payload>> &get_messages() const noexcept;

        ~DLTFile()
        {
#ifdef WIN32
            if (file_handle != nullptr && file_handle != INVALID_HANDLE_VALUE)
                CloseHandle(file_handle);
#else
            if (fd >= 0)
                close(fd);
#endif
        }

        DLTFile(const DLTFile &) = delete;
        DLTFile &operator=(const DLTFile &) = delete;

        /// @brief Get all hits of your substring and regex search.
        /// Access the hits using your regex or substring string as key for the returned map.
        /// @return An unordered_map with your substring or regex string as key and all your hits as Header, Payload pair as value
        [[nodiscard]] std::unordered_map<std::string_view, std::vector<std::pair<Header, Payload>>> get_hits() const noexcept;

        /// @return True if the DLT file was read into memory successfully, otherwise false
        [[nodiscard]] bool is_read() const noexcept;

        /// @return The size of the DLT file in bytes
        [[nodiscard]] uint64_t size() const noexcept;

        /// @return The size of each hit search
        [[nodiscard]] std::unordered_map<std::string_view, std::size_t> get_hit_sizes() const noexcept;

        // Member functions
    private:
        bool open_dlt_file() noexcept;
        void read_file() noexcept;
        void report_diagnostic(ParseDiagnostic diagnostic) const noexcept;
        void parse_from_queue(std::size_t parse_fun_lookup_idx) noexcept;

        template <bool with_raw_in_hex, bool with_substr_search, bool with_regex_search, bool with_regex_match, bool with_save_msgs>
        bool parse_with_policies(const char *buffer, std::size_t total_bytes) noexcept;

        template <bool with_substr_search, bool with_regex_search, bool with_regex_match>
        void single_thread_search(const char *payload_str, const std::streamsize &msg_idx) noexcept;

        template <bool with_substr_search, bool with_regex_search, bool with_regex_match>
        void single_thread_search(const std::string_view &payload_strv, const std::string &payload_str, const std::streamsize &msg_idx) noexcept;

        void regex_search(const std::string &str, const std::streamsize &msg_idx) noexcept;

        void regex_match(const std::string &str, const std::streamsize &msg_idx) noexcept;

        void substring_search(const std::string_view &str, const std::streamsize &msg_idx) noexcept;

        // Settings creation
    private:
        // Number of settings combinations
        static constexpr std::size_t num_settings = 32;
        using parser_function = bool (*)(DLTFile &, const char *buffer, std::size_t total_bytes);

        template <bool with_raw_in_hex, bool with_substr_search, bool with_regex_search, bool with_regex_match, bool with_save_msgs>
        static bool wrapper(DLTFile &obj, const char *buffer, std::size_t total_bytes) noexcept
        {
            return obj.parse_with_policies<with_raw_in_hex, with_substr_search, with_regex_search, with_regex_match, with_save_msgs>(buffer, total_bytes);
        }

        // Formula for indexing:
        // index = p0 * (b1 * b2 * ... * bk-1) + p1 * (b2 * b3 * ... * bk-1) + ... + pk-2 * bk-1 + pk-1
        // Formula for getting parameter X from index
        // X = floor( i / multiplier_x ) mod base_x

        // Top to bottom order for bases
        static constexpr std::size_t with_raw_in_hex_base = 2;
        static constexpr std::size_t with_substr_search_base = 2;
        static constexpr std::size_t with_regex_search_base = 2;
        static constexpr std::size_t with_regex_match_base = 2;
        static constexpr std::size_t with_save_msgs_base = 2;

        // Bottom to top order for multiplier
        static constexpr std::size_t with_save_msgs_base_mul = 1;
        static constexpr std::size_t with_regex_match_base_mul = with_save_msgs_base * with_save_msgs_base_mul;
        static constexpr std::size_t with_regex_search_base_mul = with_regex_match_base * with_regex_match_base_mul;
        static constexpr std::size_t with_substr_search_base_mul = with_regex_search_base * with_regex_search_base_mul;
        static constexpr std::size_t with_raw_in_hex_base_mul = with_substr_search_base * with_substr_search_base_mul;

        template <std::size_t idx>
        static constexpr bool with_raw_in_hex_from_idx()
        {
            return ((idx / with_raw_in_hex_base_mul) % with_raw_in_hex_base) != 0;
        }

        template <std::size_t idx>
        static constexpr bool with_substr_search_from_idx()
        {
            return ((idx / with_substr_search_base_mul) % with_substr_search_base) != 0;
        }

        template <std::size_t idx>
        static constexpr bool with_regex_search_from_idx()
        {
            return ((idx / with_regex_search_base_mul) % with_regex_search_base) != 0;
        }

        template <std::size_t idx>
        static constexpr bool with_regex_match_from_idx()
        {
            return ((idx / with_regex_match_base_mul) % with_regex_match_base) != 0;
        }

        template <std::size_t idx>
        static constexpr bool with_save_msgs_from_idx()
        {
            return ((idx / with_save_msgs_base_mul) % with_save_msgs_base) != 0;
        }

        template <std::size_t index>
        static constexpr parser_function generate_one() noexcept
        {
            constexpr bool with_raw_in_hex = with_raw_in_hex_from_idx<index>();
            constexpr bool with_substr_s = with_substr_search_from_idx<index>();
            constexpr bool with_regex_s = with_regex_search_from_idx<index>();
            constexpr bool with_regex_m = with_regex_match_from_idx<index>();
            constexpr bool with_save_mgs = with_save_msgs_from_idx<index>();
            return &wrapper<with_raw_in_hex, with_substr_s, with_regex_s, with_regex_m, with_save_mgs>;
        }

        template <std::size_t... indices>
        static std::array<parser_function, sizeof...(indices)> generate_lookup_table(std::index_sequence<indices...>)
        {
            return {generate_one<indices>()...};
        }

        inline static const std::array<parser_function, num_settings> lookup_table = generate_lookup_table(std::make_index_sequence<num_settings>{});

        // Constants
    private:
        static constexpr const char *const FILE_EXTENSION_CAPITAL = ".DLT";
        static constexpr const char *const FILE_EXTENSION = ".dlt";

        // Non-verbose payload
        static constexpr const int64_t NVB_MSG_ID_BYTES = 0x00000004;

        // Verbose payload
        static constexpr const int64_t VB_TI_BYTES = 0x0000000000000004;
        static constexpr const uint32_t VB_TI_COMP = 0x00007FF0;
        static constexpr const uint32_t VB_TI_TYLE = 0x0000000F;
        static constexpr const uint32_t VB_TI_BOOL = 0x00000010;
        static constexpr const uint32_t VB_TI_SINT = 0x00000020;
        static constexpr const uint32_t VB_TI_UINT = 0x00000040;
        static constexpr const uint32_t VB_TI_FLOA = 0x00000080;
        static constexpr const uint32_t VB_TI_ARAY = 0x00000100;
        static constexpr const uint32_t VB_TI_STRG = 0x00000200;
        static constexpr const uint32_t VB_TI_RAWD = 0x00000400;
        static constexpr const uint32_t VB_TI_VARI = 0x00000800;
        static constexpr const uint32_t VB_TI_FIXP = 0x00001000;
        static constexpr const uint32_t VB_TI_TRAI = 0x00002000;
        static constexpr const uint32_t VB_TI_STRU = 0x00004000;
        static constexpr const uint32_t VB_TI_SCOD = 0x00038000;
        static constexpr const uint32_t VB_TI_RESV = 0xFFFC0000;

        static constexpr const uint32_t VB_TI_SCOD_ASCII = 0x00000000;
        static constexpr const uint32_t VB_TI_SCOD_UTF_8 = 0x00000001;
        static constexpr const uint32_t VB_TI_SCOD_HEX = 0x00000002;
        static constexpr const uint32_t VB_TI_SCOD_BIN = 0x00000003;

    public:
        static constexpr std::size_t NUM_BUFFERS = 32;
        static constexpr std::size_t BUFFER_SIZE = 8 * 1024 * 1024;
        static constexpr std::size_t CARRY_OVER_SIZE = 8 * 1024 * 1024;

        // Member
    private:
        fs::path path = "";
        const Settings settings;
        AddSink add_sink;
        RemoveSink remove_sink;
        DiagnosticSink diagnostic_sink;
        uint64_t filesize = 0u;
        uint64_t read_bytes = 0u;
        std::streamsize msg_index = 0;
        std::vector<std::pair<Header, Payload>> messages;
        std::unordered_map<std::string_view, std::vector<std::size_t>> hit_ids;
        bool was_read = false;
        std::vector<char> carry_over_buffer;
        std::streamsize carry_over_bytes = 0;
        std::streamsize carry_over_msg_len = 0;
        FileBuffer<BUFFER_SIZE> buffers[NUM_BUFFERS]{};
        SPSCQueue<BUFFER_SIZE, NUM_BUFFERS> free_q{};
        SPSCQueue<BUFFER_SIZE, NUM_BUFFERS> full_q{};
        std::thread parser_thread;
        std::thread reader_thread;

#ifdef WIN32
        HANDLE file_handle = nullptr;
#else
        int fd = -1;
#endif
    };

    template <typename AddSink, typename RemoveSink>
    const std::vector<std::pair<Header, Payload>> &DLTFile<AddSink, RemoveSink>::get_messages() const noexcept { return messages; }

    template <typename AddSink, typename RemoveSink>
    bool DLTFile<AddSink, RemoveSink>::is_read() const noexcept { return was_read; }

    template <typename AddSink, typename RemoveSink>
    uint64_t DLTFile<AddSink, RemoveSink>::size() const noexcept { return filesize; }

    template <typename AddSink, typename RemoveSink>
    std::unordered_map<std::string_view, std::size_t> DLTFile<AddSink, RemoveSink>::get_hit_sizes() const noexcept
    {
        std::unordered_map<std::string_view, std::size_t> hit_sizes{};
        for (const auto &[k, v] : hit_ids)
        {
            hit_sizes[k] = v.size();
        }

        return hit_sizes;
    }

    template <typename AddSink, typename RemoveSink>
    std::unordered_map<std::string_view, std::vector<std::pair<Header, Payload>>> DLTFile<AddSink, RemoveSink>::get_hits() const noexcept
    {
        std::unordered_map<std::string_view, std::vector<std::pair<Header, Payload>>> out;
        for (const auto &[k, v] : hit_ids)
        {
            std::vector<std::pair<Header, Payload>> tmp = {};
            for (const auto &idx : v)
            {
                tmp.emplace_back(messages[idx]);
            }
            out[k] = tmp;
        }

        return out;
    }

    template <typename AddSink, typename RemoveSink>
    void DLTFile<AddSink, RemoveSink>::parse_from_queue(const std::size_t parse_fun_lookup_idx) noexcept
    {
        FileBuffer<BUFFER_SIZE> *buf;

        while (true)
        {
            while (!full_q.pop(buf))
            {
                _mm_pause();
            }

            // EOF
            if (buf->read_bytes == 0)
            {
                break;
            }

            // Typical case, there was carry-over
            if (carry_over_bytes > 0)
            {

                // Simple case, carry-over was larger than header, therefore we know the message length already
                if (carry_over_msg_len > 0)
                {
                    const std::streamsize bytes_to_copy = carry_over_msg_len + StorageHeader::STOHE_BYTES - carry_over_bytes;
                    if (bytes_to_copy <= 0 || bytes_to_copy > static_cast<std::streamsize>(buf->read_bytes))
                    {
                        report_diagnostic({ParseDiagnosticCode::CORRUPT_CARRY_OVER, 0, 0, bytes_to_copy});
                        carry_over_bytes = 0;
                        carry_over_msg_len = 0;
                        if (!lookup_table[parse_fun_lookup_idx](*this, buf->buffer.data(), buf->read_bytes))
                        {
                            break;
                        }
                    }
                    else
                    {
                        std::memcpy(carry_over_buffer.data() + carry_over_bytes, buf->buffer.data(), bytes_to_copy);

                        // Parse the overlapped message and remove from new buffer
                        if (!lookup_table[parse_fun_lookup_idx](*this, carry_over_buffer.data(), carry_over_msg_len + StorageHeader::STOHE_BYTES))
                        {
                            break;
                        }

                        // Parse the rest of the buffer
                        if (!lookup_table[parse_fun_lookup_idx](*this, buf->buffer.data() + bytes_to_copy, buf->read_bytes - bytes_to_copy))
                        {
                            break;
                        }
                    }

                    // Difficult case, we don't know message length yet
                }
                else
                {

                    // TODO: Cleanup and put into functions
                    constexpr std::streamsize storage_and_header_size =
                        static_cast<std::streamsize>(StorageHeader::STOHE_BYTES) + static_cast<std::streamsize>(Header::STAHE_HTYP_BYTES) + static_cast<std::streamsize>(Header::STAHE_MCNT_BYTES) + static_cast<std::streamsize>(Header::STAHE_LEN_BYTES);

                    // Copy storage and header
                    const std::streamsize remaining_header_bytes = storage_and_header_size - carry_over_bytes;

                    std::memcpy(carry_over_buffer.data() + carry_over_bytes, buf->buffer.data(), remaining_header_bytes);

                    // Parse message length
                    // TODO: This is not enough, we basically need to copy parsing loop, i.e. use functions
                    std::streamsize lenb = 18;
                    uint16_t len = byteswap(dltp::read<uint16_t>(carry_over_buffer.data(), lenb));

                    constexpr uint16_t min_hdr_len = Header::STAHE_HTYP_BYTES + Header::STAHE_MCNT_BYTES + Header::STAHE_LEN_BYTES;
                    const std::streamsize payload_bytes = static_cast<std::streamsize>(len) - 4;
                    const std::streamsize avail_in_new_buf = static_cast<std::streamsize>(buf->read_bytes) - remaining_header_bytes;
                    if (len < min_hdr_len || payload_bytes > avail_in_new_buf)
                    {
                        report_diagnostic({ParseDiagnosticCode::CORRUPT_CARRY_OVER, 0, len, payload_bytes});
                        carry_over_bytes = 0;
                        carry_over_msg_len = 0;
                        if (!lookup_table[parse_fun_lookup_idx](*this, buf->buffer.data(), buf->read_bytes))
                        {
                            break;
                        }
                    }
                    else
                    {
                        std::memcpy(carry_over_buffer.data() + 20, buf->buffer.data() + remaining_header_bytes, payload_bytes);

                        // Parse the overlapped message and remove from new buffer
                        if (!lookup_table[parse_fun_lookup_idx](*this, carry_over_buffer.data(), len + StorageHeader::STOHE_BYTES))
                        {
                            break;
                        }

                        // Parse the rest of the buffer
                        if (!lookup_table[parse_fun_lookup_idx](*this, buf->buffer.data() + remaining_header_bytes + payload_bytes, buf->read_bytes - remaining_header_bytes - payload_bytes))
                        {
                            break;
                        }
                    }
                }

                // First buffer case, or very rare case, no carry-over, just parse from the beginning
            }
            else
            {
                if (!lookup_table[parse_fun_lookup_idx](*this, buf->buffer.data(), buf->read_bytes))
                {
                    break;
                }
            }

            while (!free_q.push(buf))
            {
                _mm_pause();
            }
        }
    }

    inline std::optional<std::streamsize> find_next_stohe(
        const std::streamsize curr_offset, const char *buffer, const std::size_t total_bytes)
    {
        std::string_view sv(buffer + curr_offset, total_bytes - curr_offset);
        const char needle_buf[] = {0x44, 0x4C, 0x54, 0x01};
        const std::string_view needle{needle_buf, sizeof(needle_buf)};
        const auto it = std::search(
            sv.begin(), sv.end(),
            std::boyer_moore_horspool_searcher(needle.begin(), needle.end()));
        if (it == sv.end())
            return std::nullopt;

        return curr_offset + (it - sv.begin());
    }

    inline std::streamsize find_next_valid_stohe(
        const std::streamsize curr_offset, const char *buffer, const std::size_t total_bytes) noexcept
    {
        constexpr std::streamsize stohe_bytes = 16;
        constexpr std::streamsize ecuid_off = 12;
        constexpr std::streamsize len_off = 18; // HTYP(1) + MCNT(1) + LEN at byte 18-19
        const auto total = static_cast<std::streamsize>(total_bytes);

        for (std::streamsize i = curr_offset; i + len_off + 2 <= total; ++i)
        {
            if (static_cast<unsigned char>(buffer[i]) != 0x44u)
                continue;
            if (static_cast<unsigned char>(buffer[i + 1]) != 0x4Cu)
                continue;
            if (static_cast<unsigned char>(buffer[i + 2]) != 0x54u)
                continue;
            if (static_cast<unsigned char>(buffer[i + 3]) != 0x01u)
                continue;

            bool ecuid_ok = true;
            for (int j = 0; j < 4; ++j)
            {
                const unsigned char c = static_cast<unsigned char>(buffer[i + ecuid_off + j]);
                if (c != '\0' && (c < 0x20 || c > 0x7Eu))
                {
                    ecuid_ok = false;
                    break;
                }
            }
            if (!ecuid_ok)
                continue;

            const auto hi = static_cast<uint16_t>(static_cast<unsigned char>(buffer[i + len_off]));
            const auto lo = static_cast<uint16_t>(static_cast<unsigned char>(buffer[i + len_off + 1]));
            const uint16_t len = static_cast<uint16_t>((hi << 8) | lo);
            if (len < 4)
                continue;

            const std::streamsize next_msg = i + stohe_bytes + len;
            if (next_msg + 4 <= total)
            {
                if (static_cast<unsigned char>(buffer[next_msg]) != 0x44u)
                    continue;
                if (static_cast<unsigned char>(buffer[next_msg + 1]) != 0x4Cu)
                    continue;
                if (static_cast<unsigned char>(buffer[next_msg + 2]) != 0x54u)
                    continue;
                if (static_cast<unsigned char>(buffer[next_msg + 3]) != 0x01u)
                    continue;
            }

            return i;
        }
        return 0;
    }

    template <typename AddSink, typename RemoveSink>
    template <bool with_substr_search, bool with_regex_search, bool with_regex_match>
    void DLTFile<AddSink, RemoveSink>::single_thread_search(const char *payload_str, const std::streamsize &msg_idx) noexcept
    {
        if constexpr (with_substr_search)
        {
            std::string_view strv{payload_str};
            substring_search(strv, msg_idx);
        }

        if constexpr (with_regex_search)
        {
            std::string str{payload_str};
            regex_search(str, msg_idx);
        }

        if constexpr (with_regex_match)
        {
            std::string str{payload_str};
            regex_match(str, msg_idx);
        }
    }

    template <typename AddSink, typename RemoveSink>
    template <bool with_substr_search, bool with_regex_search, bool with_regex_match>
    void DLTFile<AddSink, RemoveSink>::single_thread_search(const std::string_view &payload_strv, const std::string &payload_str, const std::streamsize &msg_idx) noexcept
    {
        if constexpr (with_substr_search)
        {
            substring_search(payload_strv, msg_idx);
        }

        if constexpr (with_regex_search)
        {
            regex_search(payload_str, msg_idx);
        }

        if constexpr (with_regex_match)
        {
            regex_match(payload_str, msg_idx);
        }
    }

    template <typename AddSink, typename RemoveSink>
    void DLTFile<AddSink, RemoveSink>::regex_search(const std::string &str, const std::streamsize &msg_idx) noexcept
    {
        for (const auto &regx : settings.search_regex)
        {
            if (std::regex_search(str, regx.second))
            {
                hit_ids[regx.first].push_back(msg_idx);
            }
        }
    }

    template <typename AddSink, typename RemoveSink>
    void DLTFile<AddSink, RemoveSink>::regex_match(const std::string &str, const std::streamsize &msg_idx) noexcept
    {
        for (const auto &regx : settings.match_regex)
        {
            if (std::regex_match(str, regx.second))
            {
                hit_ids[regx.first].push_back(msg_idx);
            }
        }
    }

    template <typename AddSink, typename RemoveSink>
    void DLTFile<AddSink, RemoveSink>::substring_search(const std::string_view &str, const std::streamsize &msg_idx) noexcept
    {
        for (const auto &subs : settings.substr_strs)
        {
            if (contains(str, subs))
            {
                hit_ids[subs].push_back(msg_idx);
            }
        }
    }

    template <typename AddSink, typename RemoveSink>
    DLTFile<AddSink, RemoveSink>::DLTFile(const std::string &filepath, AddSink add_sink, RemoveSink remove_sink, DiagnosticSink diagnostic_sink) noexcept
        : path(filepath), settings(Settings{}), add_sink(std::move(add_sink)), remove_sink(std::move(remove_sink)), diagnostic_sink(std::move(diagnostic_sink))
    {
#ifdef DEBUG
        DEBUG_MSG("Start reading file into buffer...");
        const std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
#endif

        was_read = open_dlt_file();
        messages.reserve(settings.msg_buffer_reserve_count);

#ifdef DEBUG
        const std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        DEBUG_MSG("Reading file with " << std::to_string(filesize) << " bytes in " << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() << "s");
#endif
    }

    template <typename AddSink, typename RemoveSink>
    DLTFile<AddSink, RemoveSink>::DLTFile(const std::string &filepath, const Settings &settings, AddSink add_sink, RemoveSink remove_sink, DiagnosticSink diagnostic_sink) noexcept
        : path(filepath), settings(settings), add_sink(std::move(add_sink)), remove_sink(std::move(remove_sink)), diagnostic_sink(std::move(diagnostic_sink))
    {
#ifdef DEBUG
        DEBUG_MSG("Start reading file into buffer...");
        const std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
#endif

        was_read = open_dlt_file();
        messages.reserve(settings.msg_buffer_reserve_count);

#ifdef DEBUG
        const std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        DEBUG_MSG("Reading file with " << std::to_string(filesize) << " bytes in " << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() << "s");
#endif
    }

    template <typename AddSink, typename RemoveSink>
    void DLTFile<AddSink, RemoveSink>::report_diagnostic(ParseDiagnostic diagnostic) const noexcept
    {
        if (!diagnostic_sink)
        {
            return;
        }
        try
        {
            diagnostic_sink(diagnostic);
        }
        catch (...)
        {
            // Diagnostics must never compromise parser recovery.
        }
    }

    template <typename AddSink, typename RemoveSink>
    bool DLTFile<AddSink, RemoveSink>::open_dlt_file() noexcept
    {
        if (!fs::exists(path))
        {
            std::cerr << "The given filepath: " << path << " does not exist!" << std::endl;
            return false;
        }

        if (!fs::is_regular_file(path))
        {
            std::cerr << "The given filepath: " << path << " does not seems to be a regular file!" << std::endl;
            return false;
        }

        if (path.extension() != FILE_EXTENSION && path.extension() != FILE_EXTENSION_CAPITAL)
        {
            std::cerr << "The given filepath: " << path << " does not seems to be a dlt file!" << std::endl;
            return false;
        }

#ifdef WIN32
        file_handle = CreateFileA(
            path.string().c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_SEQUENTIAL_SCAN,
            nullptr);

        if (file_handle == INVALID_HANDLE_VALUE)
        {
            std::cerr << "Failed to create file handle for " << path << std::endl;
            return false;
        }

        LARGE_INTEGER file_size;
        if (!GetFileSizeEx(file_handle, &file_size))
        {
            std::cerr << "Failed to get file size for " << path << std::endl;
            CloseHandle(file_handle);
            file_handle = nullptr;
            return false;
        }

        filesize = static_cast<uint64_t>(file_size.QuadPart);
#else
        // TODO: Test if compiles on linux
        fd = open(path.string().c_str(), O_RDONLY);
        if (fd < 0)
        {
            std::cerr << "Failed to create file descriptor for " << path << std::endl;
            return false;
        }

        struct stat file_stat;
        if (fstat(fd, &file_stat) < 0)
        {
            std::cerr << "Failed to get file stat for " << path << std::endl;
            close(fd);
            fd = -1;
            return false;
        }

        filesize = static_cast<uint64_t>(file_stat.st_size);
#endif

        std::cout << "Opened file with " << std::to_string(filesize) << " bytes!";

        // Initialize buffers and queue to free
        for (auto &i : buffers)
        {
            FileBuffer<BUFFER_SIZE> *buffer = &i;
            free_q.push(buffer);
        }

        // Resize to carry-over size
        carry_over_buffer.resize(CARRY_OVER_SIZE);

        return true;
    }

    template <typename AddSink, typename RemoveSink>
    void DLTFile<AddSink, RemoveSink>::read_file() noexcept
    {
        // TODO: Explain entire SPSC algo
        FileBuffer<BUFFER_SIZE> *buf;

        while (true)
        {
            while (!free_q.pop(buf))
            {
                _mm_pause();
            }

#ifdef WIN32
            DWORD bytes_read = 0;
            if (!ReadFile(file_handle, buf->buffer.data(), BUFFER_SIZE, &bytes_read, nullptr) || bytes_read == 0)
            {
#else
            ssize_t bytes_read = ::read(fd, buf->buffer.data(), BUFFER_SIZE);
            if (bytes_read <= 0)
            {
#endif

                // If file is EOF, put EOF marker to last buffer
                buf->read_bytes = 0;
                while (!full_q.push(buf))
                {
                    _mm_pause();
                }
                break;
            }

            buf->read_bytes = bytes_read;
            while (!full_q.push(buf))
            {
                _mm_pause();
            }
        }
    }

    template <typename AddSink, typename RemoveSink>
    bool DLTFile<AddSink, RemoveSink>::parse() noexcept
    {
        if (!was_read)
            return false;

        std::size_t index = 0;
        index = index * 2 + (settings.with_raw_in_hex ? 1 : 0);
        index = index * 2 + (settings.with_substr() ? 1 : 0);
        index = index * 2 + (settings.with_regex_search() ? 1 : 0);
        index = index * 2 + (settings.with_regex_match() ? 1 : 0);
        index = index * 2 + (settings.save_msgs ? 1 : 0);

        reader_thread = std::thread(&DLTFile::read_file, this);
        parser_thread = std::thread(&DLTFile::parse_from_queue, this, index);

        reader_thread.join();
        parser_thread.join();

        return true;
    }

    template <typename AddSink, typename RemoveSink>
    template <bool with_raw_in_hex, bool with_substr_search, bool with_regex_search, bool with_regex_match, bool with_save_msgs>
    bool DLTFile<AddSink, RemoveSink>::parse_with_policies(const char *buffer, const std::size_t total_bytes) noexcept
    {
        std::streamsize current_offset = 0;
        std::streamsize curr_payload_len = 0;
        std::streamsize last_working_stohe_offset = 0;
        bool after_recovery = false;
        auto buffer_size = static_cast<std::streamsize>(total_bytes);

        constexpr std::streamsize storage_and_header_size =
            static_cast<std::streamsize>(StorageHeader::STOHE_BYTES) + static_cast<std::streamsize>(Header::STAHE_HTYP_BYTES) + static_cast<std::streamsize>(Header::STAHE_MCNT_BYTES) + static_cast<std::streamsize>(Header::STAHE_LEN_BYTES);
        while (current_offset < buffer_size)
        {
            std::streamsize msg_start = current_offset;

            // DEBUG_MSG("Current message offset: " + std::to_string(current_offset));
            std::streamsize bytes_left_over = buffer_size - current_offset;

            if (bytes_left_over < storage_and_header_size)
            { // Changed to < instead of <=, see if this causes issues
                report_diagnostic({ParseDiagnosticCode::INCOMPLETE_BUFFER_FRAGMENT,
                                   msg_start, 0, bytes_left_over});

                // Buffer is at the end, now handle carry over if bytes are less than header
                std::memcpy(carry_over_buffer.data(), buffer + current_offset, bytes_left_over);
                carry_over_bytes = bytes_left_over;
                carry_over_msg_len = 0; // We don't know the message len yet
                return true;
            }

            // Parse storage header
            if (byteswap(dltp::read<uint32_t>(buffer, current_offset)) != StorageHeader::STOHE_PATTERN)
            {

                std::streamsize next_stohe = find_next_valid_stohe(current_offset, buffer, total_bytes);

                bytes_left_over = buffer_size - next_stohe;
                if (next_stohe != 0)
                {
                    current_offset = next_stohe;
                    msg_start = next_stohe;

                    if constexpr (std::is_same_v<AddSink, DefaultSink> && std::is_same_v<RemoveSink, DefaultSink>)
                    {

                        if constexpr (with_save_msgs)
                        {
                            messages.pop_back();
                        }
                    }
                    else
                    {
                        remove_sink();
                    }

                    --msg_index;
                    current_offset += StorageHeader::STOHE_PATTERN_BYTES;
                    after_recovery = true;
                }
                else
                {
                    carry_over_bytes = 0;
                    carry_over_msg_len = 0;
                    return true;
                }
            }
            last_working_stohe_offset = current_offset;

            auto tmsp_s = dltp::read<uint32_t>(buffer, current_offset);
            auto tmsp_ms = dltp::read<int32_t>(buffer, current_offset);
            StorageHeader storage_header{tmsp_s, tmsp_ms, buffer, current_offset};
            current_offset += StorageHeader::STOHE_ECUID_BYTES;

            if (after_recovery)
            {
                const auto &sid = storage_header.get_ecu_id_arr();
                const bool sid_valid = std::all_of(sid.begin(), sid.end(),
                                                   [](unsigned char c)
                                                   { return c == '\0' || (c >= 0x20 && c <= 0x7E); });
                if (!sid_valid)
                {
                    const std::streamsize next = find_next_valid_stohe(current_offset, buffer, total_bytes);
                    if (next == 0)
                    {
                        carry_over_bytes = 0;
                        carry_over_msg_len = 0;
                        return true;
                    }
                    current_offset = next;
                    after_recovery = true;
                    continue;
                }
                after_recovery = false;
            }

            // Parse standard header
            // Byte 0 HTYP
            uint8_t htyp = buffer[current_offset];
            uint8_t ueh = htyp & Header::STAHE_HTYP_UEH;
            uint8_t msbf = htyp & Header::STAHE_HTYP_MSBF;
            uint8_t weid = htyp & Header::STAHE_HTYP_WEID;
            uint8_t wsid = htyp & Header::STAHE_HTYP_WSID;
            uint8_t wtms = htyp & Header::STAHE_HTYP_WTMS;

            // Not used, maybe use in the future
            // uint8_t vers = (buffer[current_offset] & STAHE_HTYP_VERS) >> STAHE_HTYP_VERS_SHFT;
            ++current_offset;

            // Byte 1 MCNT
            auto mcnt = dltp::read<uint8_t>(buffer, current_offset);

            // Byte 2-3 LEN
            uint16_t len = byteswap(dltp::read<uint16_t>(buffer, current_offset));

            // Start counting payload length from here
            curr_payload_len = len - (Header::STAHE_HTYP_BYTES + Header::STAHE_MCNT_BYTES + Header::STAHE_LEN_BYTES);

            // Check if enough bytes for the payload are left, i.e. if the file is not cut off
            if (bytes_left_over < (len + StorageHeader::STOHE_BYTES))
            {
                report_diagnostic({ParseDiagnosticCode::INCOMPLETE_BUFFER_FRAGMENT,
                                   msg_start, len, bytes_left_over});
                std::memcpy(carry_over_buffer.data(), buffer + (total_bytes - bytes_left_over), bytes_left_over);
                carry_over_bytes = bytes_left_over;
                carry_over_msg_len = len;
                return true;
            }

            // Byte 4-7 (optional) ECU ID
            std::array<char, 4> ecuid = {'N', 'U', 'L', 'L'};
            if (weid)
            {
                std::copy_n(buffer + current_offset, 4, ecuid.begin());
                current_offset += Header::STAHE_ECUID_BYTES;
                curr_payload_len -= Header::STAHE_ECUID_BYTES;
            }

            // Byte 8-11 (optional) SEID
            uint32_t session_id = 0x00;
            if (wsid)
            {
                session_id = byteswap(dltp::read<uint32_t>(buffer, current_offset));
                curr_payload_len -= Header::STAHE_SEID_BYTES;
            }

            // Byte 12-15 (optional) TMSP
            float timestamp = 0x00;
            if (wtms)
            {
                timestamp = static_cast<float>(byteswap(dltp::read<uint32_t>(buffer, current_offset)));
                curr_payload_len -= Header::STAHE_TMSP_BYTES;
            }

            // Make header object with default extended header
            Header header{msg_index, storage_header, htyp, mcnt, len, session_id, timestamp, ecuid};

            // Parse extended header (optional)
            if (ueh)
            {

                // Byte 0 MSIN
                header.extended_header.verbose = buffer[current_offset] & ExtendedHeader::EH_MSIN_VERB;
                header.extended_header.msg_type = (buffer[current_offset] & ExtendedHeader::EH_MSIN_MSTP) >> ExtendedHeader::EH_MSIN_MSTP_SHFT;
                header.extended_header.msg_type_info = (buffer[current_offset] & ExtendedHeader::EH_MSIN_MTIN) >> ExtendedHeader::EH_MSIN_MTIN_SHFT;
                ++current_offset;

                // Byte 1 NOAR
                header.extended_header.num_args = (header.extended_header.verbose) ? buffer[current_offset] : 0x00;
                ++current_offset;

                // Byte 2-5 APID
                std::copy_n(buffer + current_offset, 4, header.extended_header.application_id.begin());
                current_offset += ExtendedHeader::EH_APID_BYTES;

                // Byte 6-9 CTID
                std::copy_n(buffer + current_offset, 4, header.extended_header.context_id.begin());
                current_offset += ExtendedHeader::EH_CTID_BYTES;

                // Subtract MSIN, NOAR, APID, CTID bytes from current payload length
                curr_payload_len -= (ExtendedHeader::EH_CTID_BYTES + ExtendedHeader::EH_APID_BYTES + 1 + 1);
            }

            // Parse payload from here
            // Non-verbose payload
            if (!header.extended_header.verbose)
            {
                header.extended_header.non_verb_msg_id = msbf ? byteswap(dltp::read<uint32_t>(buffer, current_offset)) : dltp::read<uint32_t>(buffer, current_offset);
                curr_payload_len -= NVB_MSG_ID_BYTES;
                Payload payload{header.extended_header.non_verb_msg_id, Payload::Type::NON_VERBOSE};

                if (curr_payload_len > 0)
                {
                    payload.data.insert(payload.data.end(), buffer + current_offset, buffer + current_offset + curr_payload_len);
                }
                else
                {
                    report_diagnostic({ParseDiagnosticCode::INVALID_NON_VERBOSE_PAYLOAD_LENGTH,
                                       msg_start, len, curr_payload_len});
                    current_offset = msg_start + static_cast<std::streamsize>(StorageHeader::STOHE_BYTES) + static_cast<std::streamsize>(len);
                    continue;
                }

                if constexpr (std::is_same_v<AddSink, DefaultSink> && std::is_same_v<RemoveSink, DefaultSink>)
                {

                    if constexpr (with_save_msgs)
                    {
                        messages.emplace_back(header, payload);
                    }
                }
                else
                {
                    add_sink(std::move(std::pair{header, payload}));
                }

                current_offset += curr_payload_len;

                // Verbose payload
            }
            else
            {
                Payload payload{0, Payload::Type::VERBOSE};

                // Do now for each argument in the payload
                for (int i = 0; i < header.extended_header.num_args; ++i)
                {

                    // Get the type info bytes
                    uint32_t type_info = msbf ? byteswap(dltp::read<uint32_t>(buffer, current_offset)) : dltp::read<uint32_t>(buffer, current_offset);

                    // DEBUG_MSG("type info: " << std::bitset<32>(type_info));

                    uint32_t vb_tyle = type_info & VB_TI_TYLE; // TYLE in bytes
                    uint32_t vb_type = type_info & VB_TI_COMP;
                    uint32_t is_vari = type_info & VB_TI_VARI;
                    uint32_t is_fixp = type_info & VB_TI_FIXP;

                    // DEBUG_MSG("vb_type: " << std::bitset<32>(vb_type));

                    switch (vb_type)
                    {
                    case VB_TI_BOOL:
                    {
                        uint16_t vari_bool_len;
                        if (is_vari)
                        {
                            vari_bool_len = msbf ? byteswap(dltp::read<uint16_t>(buffer, current_offset)) : dltp::read<uint16_t>(buffer, current_offset);
                            payload.data.insert(payload.data.end(), buffer + current_offset, buffer + current_offset + vari_bool_len - 1);
                            payload.data.emplace_back(':');
                            payload.data.emplace_back(' ');
                            current_offset += vari_bool_len;
                        }
                        if (buffer[current_offset])
                        {
                            payload.data.insert(payload.data.end(), Payload::BOOL_TRUE_STR_VAL.begin(), Payload::BOOL_TRUE_STR_VAL.end());
                        }
                        else
                        {
                            payload.data.insert(payload.data.end(), Payload::BOOL_FALSE_STR_VAL.begin(), Payload::BOOL_FALSE_STR_VAL.end());
                        }
                        ++current_offset;
                        payload.data.emplace_back(' ');
                        break;
                    }
                    case VB_TI_SINT:
                    {
                        uint16_t vari_sint_len_name;
                        uint16_t vari_sint_len_unit;
                        std::vector<char> sint_unit_tmp_str;

                        if (is_vari)
                        {
                            vari_sint_len_name = msbf ? byteswap(dltp::read<uint16_t>(buffer, current_offset)) : dltp::read<uint16_t>(buffer, current_offset);
                            vari_sint_len_unit = msbf ? byteswap(dltp::read<uint16_t>(buffer, current_offset)) : dltp::read<uint16_t>(buffer, current_offset);
                            payload.data.insert(payload.data.end(), buffer + current_offset, buffer + current_offset + vari_sint_len_name - 1);
                            current_offset += vari_sint_len_name;
                            sint_unit_tmp_str.insert(sint_unit_tmp_str.end(), buffer + current_offset, buffer + current_offset + vari_sint_len_unit - 1);
                            current_offset += vari_sint_len_unit;
                            payload.data.emplace_back(':');
                            payload.data.emplace_back(' ');
                        }

                        if (vb_tyle == 0x01)
                        {
                            if (is_fixp)
                            {
                                auto quantization = dltp::read<float>(buffer, current_offset);
                                auto offset = msbf ? byteswap(dltp::read<int32_t>(buffer, current_offset)) : dltp::read<int32_t>(buffer, current_offset);
                                auto sint = static_cast<int8_t>((static_cast<float>(dltp::read<int8_t>(buffer, current_offset)) * quantization) + static_cast<float>(offset));
                                append_int8(payload.data, sint);
                            }
                            else
                            {
                                auto sint = dltp::read<int8_t>(buffer, current_offset);
                                append_int8(payload.data, sint);
                            }
                        }
                        else if (vb_tyle == 0x02)
                        {
                            if (is_fixp)
                            {
                                auto quantization = dltp::read<float>(buffer, current_offset);
                                auto offset = msbf ? byteswap(dltp::read<int32_t>(buffer, current_offset)) : dltp::read<int32_t>(buffer, current_offset);
                                int16_t sint = msbf ? byteswap(dltp::read<int16_t>(buffer, current_offset)) : dltp::read<int16_t>(buffer, current_offset);
                                sint = static_cast<int16_t>((static_cast<float>(sint) * quantization) + static_cast<float>(offset));
                                append_number(payload.data, sint);
                            }
                            else
                            {
                                auto sint = msbf ? byteswap(dltp::read<int16_t>(buffer, current_offset)) : dltp::read<int16_t>(buffer, current_offset);
                                append_number(payload.data, sint);
                            }
                        }
                        else if (vb_tyle == 0x03)
                        {
                            if (is_fixp)
                            {
                                auto quantization = dltp::read<float>(buffer, current_offset);
                                auto offset = msbf ? byteswap(dltp::read<int32_t>(buffer, current_offset)) : dltp::read<int32_t>(buffer, current_offset);
                                int32_t sint = msbf ? byteswap(dltp::read<int32_t>(buffer, current_offset)) : dltp::read<int32_t>(buffer, current_offset);
                                sint = static_cast<int32_t>((static_cast<float>(sint) * quantization) + static_cast<float>(offset));
                                append_number(payload.data, sint);
                            }
                            else
                            {
                                auto sint = msbf ? byteswap(dltp::read<int32_t>(buffer, current_offset)) : dltp::read<int32_t>(buffer, current_offset);
                                append_number(payload.data, sint);
                            }
                        }
                        else if (vb_tyle == 0x04)
                        {
                            if (is_fixp)
                            {
                                auto quantization = dltp::read<float>(buffer, current_offset);
                                auto offset = msbf ? byteswap(dltp::read<int64_t>(buffer, current_offset)) : dltp::read<int64_t>(buffer, current_offset);
                                auto sint = msbf ? byteswap(dltp::read<int64_t>(buffer, current_offset)) : dltp::read<int64_t>(buffer, current_offset);
                                sint = static_cast<int64_t>((static_cast<double>(sint) * quantization) + static_cast<double>(offset));
                                append_number(payload.data, sint);
                            }
                            else
                            {
                                auto sint = msbf ? byteswap(dltp::read<int64_t>(buffer, current_offset)) : dltp::read<int64_t>(buffer, current_offset);
                                append_number(payload.data, sint);
                            }
                        }
                        // Note: 128-bit is not supported yet!

                        payload.data.insert(payload.data.end(), sint_unit_tmp_str.begin(), sint_unit_tmp_str.end());
                        payload.data.emplace_back(' ');
                        break;
                    }
                    case VB_TI_UINT:
                    {
                        uint16_t vari_uint_len_name;
                        uint16_t vari_uint_len_unit;
                        std::vector<char> uint_unit_tmp_str;

                        if (is_vari)
                        {
                            vari_uint_len_name = msbf ? byteswap(dltp::read<uint16_t>(buffer, current_offset)) : dltp::read<uint16_t>(buffer, current_offset);
                            vari_uint_len_unit = msbf ? byteswap(dltp::read<uint16_t>(buffer, current_offset)) : dltp::read<uint16_t>(buffer, current_offset);
                            payload.data.insert(payload.data.end(), buffer + current_offset, buffer + current_offset + vari_uint_len_name - 1);
                            current_offset += vari_uint_len_name;
                            uint_unit_tmp_str.insert(uint_unit_tmp_str.end(), buffer + current_offset, buffer + current_offset + vari_uint_len_unit - 1);
                            current_offset += vari_uint_len_unit;
                            payload.data.emplace_back(':');
                            payload.data.emplace_back(' ');
                        }

                        if (vb_tyle == 0x01)
                        {
                            if (is_fixp)
                            {
                                auto quantization = dltp::read<float>(buffer, current_offset);
                                auto offset = msbf ? byteswap(dltp::read<int32_t>(buffer, current_offset)) : dltp::read<int32_t>(buffer, current_offset);
                                auto uint = static_cast<uint8_t>((static_cast<float>(dltp::read<uint8_t>(buffer, current_offset)) * quantization) + static_cast<float>(offset));
                                append_uint8(payload.data, uint);
                            }
                            else
                            {
                                auto uint = dltp::read<uint8_t>(buffer, current_offset);
                                append_uint8(payload.data, uint);
                            }
                        }
                        else if (vb_tyle == 0x02)
                        {
                            if (is_fixp)
                            {
                                auto quantization = dltp::read<float>(buffer, current_offset);
                                auto offset = msbf ? byteswap(dltp::read<int32_t>(buffer, current_offset)) : dltp::read<int32_t>(buffer, current_offset);
                                uint16_t uint = msbf ? byteswap(dltp::read<uint16_t>(buffer, current_offset)) : dltp::read<uint16_t>(buffer, current_offset);
                                uint = static_cast<uint16_t>((static_cast<float>(uint) * quantization) + static_cast<float>(offset));
                                append_number(payload.data, uint);
                            }
                            else
                            {
                                auto uint = msbf ? byteswap(dltp::read<uint16_t>(buffer, current_offset)) : dltp::read<uint16_t>(buffer, current_offset);
                                append_number(payload.data, uint);
                            }
                        }
                        else if (vb_tyle == 0x03)
                        {
                            if (is_fixp)
                            {
                                auto quantization = dltp::read<float>(buffer, current_offset);
                                auto offset = msbf ? byteswap(dltp::read<int32_t>(buffer, current_offset)) : dltp::read<int32_t>(buffer, current_offset);
                                uint32_t uint = msbf ? byteswap(dltp::read<uint32_t>(buffer, current_offset)) : dltp::read<uint32_t>(buffer, current_offset);
                                uint = static_cast<uint32_t>((static_cast<float>(uint) * quantization) + static_cast<float>(offset));
                                append_number(payload.data, uint);
                            }
                            else
                            {
                                auto uint = msbf ? byteswap(dltp::read<uint32_t>(buffer, current_offset)) : dltp::read<uint32_t>(buffer, current_offset);
                                append_number(payload.data, uint);
                            }
                        }
                        else if (vb_tyle == 0x04)
                        {
                            if (is_fixp)
                            {
                                auto quantization = dltp::read<float>(buffer, current_offset);
                                auto offset = msbf ? byteswap(dltp::read<int64_t>(buffer, current_offset)) : dltp::read<int64_t>(buffer, current_offset);
                                auto uint = msbf ? byteswap(dltp::read<uint64_t>(buffer, current_offset)) : dltp::read<uint64_t>(buffer, current_offset);
                                uint = static_cast<uint64_t>((static_cast<double>(uint) * quantization) + static_cast<double>(offset));
                                append_number(payload.data, uint);
                            }
                            else
                            {
                                auto uint = msbf ? byteswap(dltp::read<uint64_t>(buffer, current_offset)) : dltp::read<uint64_t>(buffer, current_offset);
                                append_number(payload.data, uint);
                            }
                        }
                        // Note: 128-bit is not supported yet!

                        payload.data.insert(payload.data.end(), uint_unit_tmp_str.begin(), uint_unit_tmp_str.end());
                        payload.data.emplace_back(' ');
                        break;
                    }
                    case VB_TI_FLOA:
                    {
                        uint16_t vari_float_len_name;
                        uint16_t vari_float_len_unit;
                        std::vector<char> float_unit_tmp_str;

                        if (is_vari)
                        {
                            vari_float_len_name = msbf ? byteswap(dltp::read<uint16_t>(buffer, current_offset)) : dltp::read<uint16_t>(buffer, current_offset);
                            vari_float_len_unit = msbf ? byteswap(dltp::read<uint16_t>(buffer, current_offset)) : dltp::read<uint16_t>(buffer, current_offset);
                            payload.data.insert(payload.data.end(), buffer + current_offset, buffer + current_offset + vari_float_len_name - 1);
                            current_offset += vari_float_len_name;
                            float_unit_tmp_str.insert(float_unit_tmp_str.end(), buffer + current_offset, buffer + current_offset + vari_float_len_unit - 1);
                            current_offset += vari_float_len_unit;
                            payload.data.emplace_back(':');
                            payload.data.emplace_back(' ');
                        }

                        // Note: 16-bit is not supported yet!
                        if (vb_tyle == 0x03)
                        {
                            auto floa = dltp::read<float>(buffer, current_offset);
                            append_number(payload.data, floa);
                        }
                        else if (vb_tyle == 0x04)
                        {
                            auto doub = dltp::read<double>(buffer, current_offset);
                            append_number(payload.data, doub);
                        }
                        // Note: 128-bit is not supported yet!

                        payload.data.insert(payload.data.end(), float_unit_tmp_str.begin(), float_unit_tmp_str.end());
                        payload.data.emplace_back(' ');
                        break;
                    }
                    case VB_TI_ARAY:
                    {
                        // TODO: Implement
                        std::cerr << "Array type found but not supported yet!" << std::endl;
                        break;
                    }
                    case VB_TI_STRG:
                    {
                        uint16_t strg_len;
                        uint16_t vari_strg_len;
                        payload.str_coding = (type_info & VB_TI_SCOD) >> 0x0000000F;
                        strg_len = msbf ? byteswap(dltp::read<uint16_t>(buffer, current_offset)) : dltp::read<uint16_t>(buffer, current_offset);
                        if (is_vari)
                        {
                            vari_strg_len = msbf ? byteswap(dltp::read<uint16_t>(buffer, current_offset)) : dltp::read<uint16_t>(buffer, current_offset);
                            if (vari_strg_len > 0)
                            {
                                payload.data.insert(payload.data.end(), buffer + current_offset, buffer + current_offset + vari_strg_len - 1);
                            }
                            payload.data.emplace_back(':');
                            payload.data.emplace_back(' ');
                            current_offset += vari_strg_len;
                        }
                        if (strg_len > 0)
                        {
                            payload.data.insert(payload.data.end(), buffer + current_offset, buffer + current_offset + strg_len);
                            current_offset += strg_len;
                            payload.data.back() = ' ';
                        }
                        else
                        {
                            payload.data.emplace_back(' ');
                        }
                        break;
                    }
                    case VB_TI_RAWD:
                    {
                        uint16_t raw_len;
                        uint16_t vari_raw_len;
                        raw_len = msbf ? byteswap(dltp::read<uint16_t>(buffer, current_offset)) : dltp::read<uint16_t>(buffer, current_offset);
                        if (is_vari)
                        {
                            vari_raw_len = msbf ? byteswap(dltp::read<uint16_t>(buffer, current_offset)) : dltp::read<uint16_t>(buffer, current_offset);
                            payload.data.insert(payload.data.end(), buffer + current_offset, buffer + current_offset + vari_raw_len - 1);
                            payload.data.emplace_back(':');
                            payload.data.emplace_back(' ');
                            current_offset += vari_raw_len;
                        }

                        if constexpr (with_raw_in_hex)
                        {
                            appends_str_hex(payload.data, buffer + current_offset, raw_len);
                            payload.data.emplace_back(' ');
                        }
                        else
                        {
                            payload.data.emplace_back(' ');
                        }

                        current_offset += raw_len;
                        break;
                    }
                    case VB_TI_TRAI:
                    {
                        uint16_t trai_len;
                        payload.str_coding = (type_info & VB_TI_SCOD) >> 0x0000000F;
                        trai_len = msbf ? byteswap(dltp::read<uint16_t>(buffer, current_offset)) : dltp::read<uint16_t>(buffer, current_offset);
                        payload.data.insert(payload.data.end(), buffer + current_offset, buffer + current_offset + trai_len);
                        current_offset += trai_len;
                        payload.data.emplace_back(' ');
                        break;
                    }
                    case VB_TI_STRU:
                    {
                        // TODO: Implement
                        std::cerr << "Struct type found but not supported yet!" << std::endl;
                        break;
                    }
                    default:
                    {
                        break;
                    }
                    }
                }

                std::string_view v = payload.to_view();
                std::string s = payload.to_string();

                if constexpr (with_substr_search || with_regex_search || with_regex_match)
                {
                    single_thread_search<with_substr_search, with_regex_search, with_regex_match>(v, s, msg_index);
                }

                if constexpr (std::is_same_v<AddSink, DefaultSink> && std::is_same_v<RemoveSink, DefaultSink>)
                {

                    if constexpr (with_save_msgs)
                    {
                        messages.emplace_back(header, payload);
                    }
                }
                else
                {
                    add_sink(std::move(std::pair{header, payload}));
                }
            }

            // DEBUG_MSG(messages.back().first.to_string() + " " + messages.back().second.to_string());

            ++msg_index;
        }

        // Perfect buffer alignment, no carry-over
        carry_over_bytes = 0;
        carry_over_msg_len = 0;

        // DEBUG_MSG("Finished parsing with msg_idx: " + std::to_string(msg_index) + " and message size: " + std::to_string(messages.size()));

        return true;
    }

    template <typename AddSink, typename RemoveSink>
    bool DLTFile<AddSink, RemoveSink>::save_messages(const std::string &filepath) const noexcept
    {
        std::ofstream outfile(filepath);
        if (!outfile)
        {
            std::cerr << "Failed to create file!" << std::endl;
            return false;
        }

        for (const auto &[fst, snd] : messages)
        {
            Header header = fst;
            Payload payload = snd;
            outfile << header.to_view() << " " << payload.to_view() << std::endl;
        }

        outfile.close();
        return true;
    }

    template <typename AddSink, typename RemoveSink>
    bool DLTFile<AddSink, RemoveSink>::save_hits(const std::string &filepath) const noexcept
    {
        std::ofstream outfile(filepath);
        if (!outfile)
        {
            std::cerr << "Failed to create file!" << std::endl;
            return false;
        }

        for (const auto &[k, v] : hit_ids)
        {
            for (const auto msg_id : v)
            {
                Header header = messages[msg_id].first;
                Payload payload = messages[msg_id].second;
                outfile << k << ": " << header.to_view() << " " << payload.to_view() << std::endl;
            }
        }

        outfile.close();
        return true;
    }

} // namespace dltp

#endif // DLTPARSER_DLTPARSER_H