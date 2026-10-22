// SPDX-License-Identifier: MIT
// Copyright (c) 2026 MBition GmbH

#ifndef DLTPARSER_DLTHEADER_H
#define DLTPARSER_DLTHEADER_H

#include <cstdint>
#include <array>

#include "utils.h"

namespace dltp
{

    std::array<char, 4> to_ecu_id(const char *msg_buffer, const std::streamsize &offset) noexcept;

    /// @brief The StorageHeader object exists for each message. It stores the timestamp and the ecu id of the message.
    class StorageHeader
    {

        // Interface
    public:
        friend class Header;

        StorageHeader() = delete;
        ~StorageHeader() = default;

        /// @brief Make the StorageHeader. This is only needed during parsing. There should be no need to create a StorageHeader by yourself.
        /// @param tmsp_s The seconds of the timestamp
        /// @param tmsp_ms The milliseconds of the timestamp
        /// @param msg_buffer The message buffer to create the ecu id from
        /// @param offset The offset of the ecu id used for creation
        StorageHeader(uint32_t tmsp_s, int32_t tmsp_ms, const char *msg_buffer, const std::streamsize &offset) noexcept
            : timestamp_s(tmsp_s), timestamp_ms(tmsp_ms), ecu_id(to_ecu_id(msg_buffer, offset)) {};

        // Make copy constructor
        StorageHeader(const StorageHeader &) noexcept = default;
        StorageHeader &operator=(const StorageHeader &) noexcept = default;

        // Make default move constructor
        StorageHeader(StorageHeader &&) noexcept = default;
        StorageHeader &operator=(StorageHeader &&) noexcept = default;

        /// @return The human readable timestamp string
        [[nodiscard]] std::string get_timestamp_str() const noexcept;

        /// @return The human readable timestamp as view
        [[nodiscard]] std::string_view get_timestamp_view() const noexcept;

        /// @return The seconds of the timestamp
        [[nodiscard]] uint32_t get_timestamp_s() const noexcept;

        /// @return The milliseconds of the timestamp
        [[nodiscard]] int32_t get_timestamp_ms() const noexcept;

        /// @return The ecu id as c-string
        [[nodiscard]] const char *get_ecu_id_cstr() const noexcept;

        /// @return The ecu id as view
        [[nodiscard]] std::string_view get_ecu_id_view() const noexcept;

        /// @return The ecu id string
        [[nodiscard]] std::string get_ecu_id_str() const noexcept;

        /// @return The ecu id as char array
        [[nodiscard]] const std::array<char, 4> &get_ecu_id_arr() const noexcept;

        // Class routines
    private:
        void append_storage_header_str(std::vector<char> &buf) const noexcept;
        void append_timestamp(std::vector<char> &buf) const noexcept;

        void append_localtime(std::vector<char> &buf) const noexcept;
        void append_utc(std::vector<char> &buf) const noexcept;

        // Public member
    public:
        uint32_t timestamp_s;
        int32_t timestamp_ms;
        std::array<char, 4> ecu_id;

        // Public constants
    public:
        static constexpr uint32_t STOHE_PATTERN = 0x444C5401;
        static constexpr int64_t STOHE_PATTERN_BYTES = 0x0000000000000004;
        static constexpr int64_t STOHE_TIMESTAMP_SEC_BYTES = 0x0000000000000004;
        static constexpr int64_t STOHE_TIMESTAMP_MICSEC_BYTES = 0x0000000000000004;
        static constexpr int64_t STOHE_ECUID_BYTES = 0x0000000000000004;
        static constexpr int64_t STOHE_BYTES = 0x0000000000000010;

        // Private constants
    private:
        static constexpr std::array<char, 7> time_arr = {',', ' ', 'T', 'i', 'm', 'e', '='};
        static constexpr std::size_t date_buf_tmp_len = 20;
        static constexpr const char *const date_format = "%Y/%m/%d %H:%M:%S";
    };

    inline std::string StorageHeader::get_timestamp_str() const noexcept
    {
        std::vector<char> buf;

        // Append the timestamp
        append_str_fixed(buf, time_arr);
        append_timestamp(buf);

        return {buf.data(), buf.size()};
    }

    inline std::string_view StorageHeader::get_timestamp_view() const noexcept
    {
        std::vector<char> buf;

        // Append the timestamp
        append_str_fixed(buf, time_arr);
        append_timestamp(buf);

        return {buf.data(), buf.size()};
    }

    inline uint32_t StorageHeader::get_timestamp_s() const noexcept { return timestamp_s; }
    inline int32_t StorageHeader::get_timestamp_ms() const noexcept { return timestamp_ms; }

    inline const char *StorageHeader::get_ecu_id_cstr() const noexcept { return ecu_id.data(); }
    inline std::string_view StorageHeader::get_ecu_id_view() const noexcept { return {ecu_id.data(), ecu_id.size()}; }
    inline std::string StorageHeader::get_ecu_id_str() const noexcept { return {ecu_id.data(), ecu_id.size()}; }
    inline const std::array<char, 4> &StorageHeader::get_ecu_id_arr() const noexcept { return ecu_id; }

    inline void StorageHeader::append_storage_header_str(std::vector<char> &buf) const noexcept
    {
        // Note: ecu_id is not appened here yet.

        // Append the timestamp
        append_str_fixed(buf, time_arr);
        append_timestamp(buf);
    }

    inline void StorageHeader::append_timestamp(std::vector<char> &buf) const noexcept
    {
        // TODO: Include a switch between localtime and utc (which may be faster)
        append_localtime(buf);
        // append_utc(buf);
    }

    inline void StorageHeader::append_localtime(std::vector<char> &buf) const noexcept
    {
        std::time_t time = timestamp_s;
        std::tm time_info{};
        char tmp_buf[date_buf_tmp_len];

#ifdef _WIN32
        localtime_s(&time_info, &time);
#else
        localtime_r(&time, &time_info);
#endif

        std::strftime(tmp_buf, date_buf_tmp_len, date_format, &time_info);
        buf.insert(buf.end(), tmp_buf, tmp_buf + date_buf_tmp_len);
        buf.pop_back(); // Remove null termination character
        buf.emplace_back('.');
        append_number<int32_t>(buf, timestamp_ms);
    }

    inline void StorageHeader::append_utc(std::vector<char> &buf) const noexcept
    {
        std::time_t time = timestamp_s;
        std::tm time_info{};
        char tmp_buf[date_buf_tmp_len - 1];

#ifdef _WIN32
        gmtime_s(&time_info, &time);
#else
        gmtime_r(&time, &time_info);
#endif

        auto write2 = [](char *dest, int val)
        {
            dest[0] = static_cast<char>('0' + (val / 10));
            dest[1] = static_cast<char>('0' + (val % 10));
        };

        int year = time_info.tm_year + 1900;
        tmp_buf[0] = static_cast<char>('0' + (year / 1000 % 10));
        tmp_buf[1] = static_cast<char>('0' + (year / 100 % 10));
        tmp_buf[2] = static_cast<char>('0' + (year / 10 % 10));
        tmp_buf[3] = static_cast<char>('0' + (year % 10));
        tmp_buf[4] = '/';

        write2(tmp_buf + 5, time_info.tm_mon + 1);
        tmp_buf[7] = '/';

        write2(tmp_buf + 8, time_info.tm_mday);
        tmp_buf[10] = ' ';

        write2(tmp_buf + 11, time_info.tm_hour);
        tmp_buf[13] = ':';

        write2(tmp_buf + 14, time_info.tm_min);
        tmp_buf[16] = ':';

        write2(tmp_buf + 17, time_info.tm_sec);

        buf.insert(buf.end(), tmp_buf, tmp_buf + date_buf_tmp_len - 1);
        buf.emplace_back('.');
        append_number<int32_t>(buf, timestamp_ms);
    }

    /// @brief The ExtendedHeader object exists for message with an ExtendedHeader. It stores extended information of the message.
    class ExtendedHeader
    {

        // Interface
    public:
        friend class Header;

        ExtendedHeader() noexcept = default;
        ~ExtendedHeader() = default;

        /// @brief Make the ExtendedHeader. This is only needed during parsing. There should be no need to create a ExtendedHeader by yourself.
        /// @param verb The verbosity
        /// @param mstp The message type
        /// @param mtin The message type info
        /// @param noar The number of arguments
        /// @param apid The application id
        /// @param ctid The content id
        ExtendedHeader(uint8_t verb, uint8_t mstp, uint8_t mtin, int8_t noar, std::array<char, 4> apid, std::array<char, 4> ctid) noexcept : verbose(verb), msg_type(mstp), msg_type_info(mtin), num_args(noar), application_id(apid), context_id(ctid) {}

        // Make copy constructor
        ExtendedHeader(const ExtendedHeader &) noexcept = default;
        ExtendedHeader &operator=(const ExtendedHeader &) noexcept = default;

        // Make default move constructor
        ExtendedHeader(ExtendedHeader &&) noexcept = default;
        ExtendedHeader &operator=(ExtendedHeader &&) noexcept = default;

        /// @return The raw verbosity
        [[nodiscard]] uint8_t get_verbosity() const noexcept;

        /// @return The human readable verbosity c-string
        [[nodiscard]] const char *get_verbosity_cstr() const noexcept;

        /// @return The human readable verbosity string
        [[nodiscard]] std::string get_verbosity_str() const noexcept;

        /// @return The human readable verbosity as view
        [[nodiscard]] std::string_view get_verbosity_view() const noexcept;

        /// @return The raw message type
        [[nodiscard]] uint8_t get_msg_type() const noexcept;

        /// @return The human readable message type c-string
        [[nodiscard]] const char *get_msg_type_cstr() const noexcept;

        /// @return The human readable message type string
        [[nodiscard]] std::string get_msg_type_str() const noexcept;

        /// @return The human readable message type as view
        [[nodiscard]] std::string_view get_msg_type_view() const noexcept;

        /// @return The raw message type info
        [[nodiscard]] uint8_t get_msg_type_info() const noexcept;

        /// @return The human readable message type info c-string
        [[nodiscard]] const char *get_msg_type_info_cstr() const noexcept;

        /// @return The human readable message type info string
        [[nodiscard]] std::string get_msg_type_info_str() const noexcept;

        /// @return The human readable message type info as view
        [[nodiscard]] std::string_view get_msg_type_info_view() const noexcept;

        /// @return The number of arguments
        [[nodiscard]] uint8_t get_num_args() const noexcept;

        /// @return The message id of a non-verbose message
        [[nodiscard]] uint32_t get_non_verb_msg_id() const noexcept;

        /// @return The application id c-string
        [[nodiscard]] const char *get_apid_cstr() const noexcept;

        /// @return The application id string
        [[nodiscard]] std::string get_apid_str() const noexcept;

        /// @return The application id as view
        [[nodiscard]] std::string_view get_apid_view() const noexcept;

        /// @return The application id as char array
        [[nodiscard]] const std::array<char, 4> &get_apid_arr() const noexcept;

        /// @return The content id c-string
        [[nodiscard]] const char *get_ctid_cstr() const noexcept;

        /// @return The content id string
        [[nodiscard]] std::string get_ctid_str() const noexcept;

        /// @return The content id as view
        [[nodiscard]] std::string_view get_ctid_view() const noexcept;

        /// @return The content id as char array
        [[nodiscard]] const std::array<char, 4> &get_ctid_arr() const noexcept;

        // Class routines
    private:
        void append_extended_header_str(std::vector<char> &buf) const noexcept;

        // Public member
    public:
        uint8_t verbose = 0x00;
        uint8_t msg_type = 0x00;
        uint8_t msg_type_info = 0x00;
        uint8_t num_args = 0x00;
        uint32_t non_verb_msg_id = 0x00;
        std::array<char, 4> application_id = {'N', 'U', 'L', 'L'};
        std::array<char, 4> context_id = {'N', 'U', 'L', 'L'};

        // Public constants
    public:
        static constexpr uint8_t EH_MSIN_BYTES = 0x01;
        static constexpr uint8_t EH_MSIN_VERB = 0x01;
        static constexpr uint8_t EH_MSIN_MSTP = 0x0E;
        static constexpr uint8_t EH_MSIN_MSTP_SHFT = 0x01;
        static constexpr uint8_t EH_MSIN_MTIN = 0xF0;
        static constexpr uint8_t EH_MSIN_MTIN_SHFT = 0x04;

        static constexpr uint8_t EH_NOAR_BYTES = 0x01;
        static constexpr int64_t EH_APID_BYTES = 0x0000000000000004;
        static constexpr int64_t EH_CTID_BYTES = 0x0000000000000004;

        // Private constants
    private:
        static constexpr uint8_t MSTP_LOG_IDF = 0x00;
        static constexpr uint8_t MSTP_APP_TRACE_IDF = 0x01;
        static constexpr uint8_t MSTP_NW_TRACE_IDF = 0x02;
        static constexpr uint8_t MSTP_CONTROL_IDF = 0x03;
        static inline const std::unordered_map<uint8_t, std::string_view> mstp_map = {
            {MSTP_LOG_IDF, "Log"},
            {MSTP_APP_TRACE_IDF, "Trace"},
            {MSTP_NW_TRACE_IDF, "Network"},
            {MSTP_CONTROL_IDF, "Control"}};

        static constexpr uint8_t MTIN_LOG_FATAL_IDF = 0x01;
        static constexpr uint8_t MTIN_LOG_ERROR_IDF = 0x02;
        static constexpr uint8_t MTIN_LOG_WARN_IDF = 0x03;
        static constexpr uint8_t MTIN_LOG_INFO_IDF = 0x04;
        static constexpr uint8_t MTIN_LOG_DEBUG_IDF = 0x05;
        static constexpr uint8_t MTIN_LOG_VERBOSE_IDF = 0x06;
        static inline const std::unordered_map<uint8_t, std::string_view> mtin_log_map = {
            {MTIN_LOG_FATAL_IDF, "Fatal"},
            {MTIN_LOG_ERROR_IDF, "Error"},
            {MTIN_LOG_WARN_IDF, "Warn"},
            {MTIN_LOG_INFO_IDF, "Info"},
            {MTIN_LOG_DEBUG_IDF, "Debug"},
            {MTIN_LOG_VERBOSE_IDF, "Verbose"}};

        static constexpr uint8_t MTIN_APP_TRACE_VAR_IDF = 0x01;
        static constexpr uint8_t MTIN_APP_TRACE_FUN_IN_IDF = 0x02;
        static constexpr uint8_t MTIN_APP_TRACE_FUN_OUT_IDF = 0x03;
        static constexpr uint8_t MTIN_APP_TRACE_STATE_IDF = 0x04;
        static constexpr uint8_t MTIN_APP_TRACE_VFB_IDF = 0x05;
        static inline const std::unordered_map<uint8_t, std::string_view> mtin_app_trace_map = {
            {MTIN_APP_TRACE_VAR_IDF, "Variable"},
            {MTIN_APP_TRACE_FUN_IN_IDF, "Function In"},
            {MTIN_APP_TRACE_FUN_OUT_IDF, "Function Out"},
            {MTIN_APP_TRACE_STATE_IDF, "State"},
            {MTIN_APP_TRACE_VFB_IDF, "VFB"}};

        static constexpr uint8_t MTIN_NW_TRACE_IPC_IDF = 0x01;
        static constexpr uint8_t MTIN_NW_TRACE_CAN_IDF = 0x02;
        static constexpr uint8_t MTIN_NW_TRACE_FLEXRAY_IDF = 0x03;
        static constexpr uint8_t MTIN_NW_TRACE_MOST_IDF = 0x04;
        static constexpr uint8_t MTIN_NW_TRACE_ETHERNET_IDF = 0x05;
        static constexpr uint8_t MTIN_NW_TRACE_SOMEIP_IDF = 0x06;
        static constexpr uint8_t MTIN_NW_TRACE_USER_IDF_01 = 0x07;
        static constexpr uint8_t MTIN_NW_TRACE_USER_IDF_02 = 0x08;
        static constexpr uint8_t MTIN_NW_TRACE_USER_IDF_03 = 0x09;
        static constexpr uint8_t MTIN_NW_TRACE_USER_IDF_04 = 0x0A;
        static constexpr uint8_t MTIN_NW_TRACE_USER_IDF_05 = 0x0B;
        static constexpr uint8_t MTIN_NW_TRACE_USER_IDF_06 = 0x0C;
        static constexpr uint8_t MTIN_NW_TRACE_USER_IDF_07 = 0x0D;
        static constexpr uint8_t MTIN_NW_TRACE_USER_IDF_08 = 0x0E;
        static constexpr uint8_t MTIN_NW_TRACE_USER_IDF_09 = 0x0F;
        static inline const std::unordered_map<uint8_t, std::string_view> mtin_nw_trace_map = {
            {MTIN_NW_TRACE_IPC_IDF, "IPC"},
            {MTIN_NW_TRACE_CAN_IDF, "CAN"},
            {MTIN_NW_TRACE_FLEXRAY_IDF, "FlexRay"},
            {MTIN_NW_TRACE_MOST_IDF, "Most"},
            {MTIN_NW_TRACE_ETHERNET_IDF, "Ethernet"},
            {MTIN_NW_TRACE_SOMEIP_IDF, "SOME/IP"},
            {MTIN_NW_TRACE_USER_IDF_01, "User 07"},
            {MTIN_NW_TRACE_USER_IDF_02, "User 08"},
            {MTIN_NW_TRACE_USER_IDF_03, "User 09"},
            {MTIN_NW_TRACE_USER_IDF_04, "User 0A"},
            {MTIN_NW_TRACE_USER_IDF_05, "User 0B"},
            {MTIN_NW_TRACE_USER_IDF_06, "User 0C"},
            {MTIN_NW_TRACE_USER_IDF_07, "User 0D"},
            {MTIN_NW_TRACE_USER_IDF_08, "User 0E"},
            {MTIN_NW_TRACE_USER_IDF_09, "User 0F"}};

        static constexpr uint8_t MTIN_CONTROL_REQ_IDF = 0x01;
        static constexpr uint8_t MTIN_CONTROL_RESP_IDF = 0x02;
        static constexpr uint8_t MTIN_CONTROL_TIME_IDF = 0x03;
        static inline const std::unordered_map<uint8_t, std::string> mtin_control_map = {
            {MTIN_CONTROL_REQ_IDF, "Request"},
            {MTIN_CONTROL_RESP_IDF, "Response"},
            {MTIN_CONTROL_TIME_IDF, "Time"}};

        static inline const std::unordered_map<uint8_t, const std::unordered_map<uint8_t, std::string_view>> mstp_mtin_map = {
            {MSTP_LOG_IDF, {{MTIN_LOG_FATAL_IDF, "Fatal"}, {MTIN_LOG_ERROR_IDF, "Error"}, {MTIN_LOG_WARN_IDF, "Warn"}, {MTIN_LOG_INFO_IDF, "Info"}, {MTIN_LOG_DEBUG_IDF, "Debug"}, {MTIN_LOG_VERBOSE_IDF, "Verbose"}}},
            {MSTP_APP_TRACE_IDF, {{MTIN_APP_TRACE_VAR_IDF, "Variable"}, {MTIN_APP_TRACE_FUN_IN_IDF, "Function In"}, {MTIN_APP_TRACE_FUN_OUT_IDF, "Function Out"}, {MTIN_APP_TRACE_STATE_IDF, "State"}, {MTIN_APP_TRACE_VFB_IDF, "VFB"}}},
            {MSTP_NW_TRACE_IDF, {{MTIN_NW_TRACE_IPC_IDF, "IPC"}, {MTIN_NW_TRACE_CAN_IDF, "CAN"}, {MTIN_NW_TRACE_FLEXRAY_IDF, "FlexRay"}, {MTIN_NW_TRACE_MOST_IDF, "Most"}, {MTIN_NW_TRACE_ETHERNET_IDF, "Ethernet"}, {MTIN_NW_TRACE_SOMEIP_IDF, "SOME/IP"}, {MTIN_NW_TRACE_USER_IDF_01, "User 07"}, {MTIN_NW_TRACE_USER_IDF_02, "User 08"}, {MTIN_NW_TRACE_USER_IDF_03, "User 09"}, {MTIN_NW_TRACE_USER_IDF_04, "User 0A"}, {MTIN_NW_TRACE_USER_IDF_05, "User 0B"}, {MTIN_NW_TRACE_USER_IDF_06, "User 0C"}, {MTIN_NW_TRACE_USER_IDF_07, "User 0D"}, {MTIN_NW_TRACE_USER_IDF_08, "User 0E"}, {MTIN_NW_TRACE_USER_IDF_09, "User 0F"}}},
            {MSTP_CONTROL_IDF, {{MTIN_CONTROL_REQ_IDF, "Request"}, {MTIN_CONTROL_RESP_IDF, "Response"}, {MTIN_CONTROL_TIME_IDF, "Time"}}}};

        static constexpr std::array<char, 4> null_arr = {'N', 'U', 'L', 'L'};
        static constexpr std::array<char, 7> apid_arr = {',', ' ', 'A', 'p', 'i', 'd', '='};
        static constexpr std::array<char, 7> ctid_arr = {',', ' ', 'C', 't', 'i', 'd', '='};
        static constexpr std::array<char, 7> type_arr = {',', ' ', 'T', 'y', 'p', 'e', '='};
        static constexpr std::array<char, 10> subtype_arr = {',', ' ', 'S', 'u', 'b', 't', 'y', 'p', 'e', '='};
        static constexpr std::array<char, 7> mode_arr = {',', ' ', 'M', 'o', 'd', 'e', '='};
        static constexpr std::array<char, 7> verb_arr = {'v', 'e', 'r', 'b', 'o', 's', 'e'};
        static constexpr std::array<char, 9> msgid_arr_0 = {',', ' ', 'M', 's', 'g', 'I', 'D', '=', '0'};
        static constexpr std::array<char, 11> non_verb_arr = {'n', 'o', 'n', '-', 'v', 'e', 'r', 'b', 'o', 's', 'e'};
        static constexpr std::array<char, 8> msgid_arr = {',', ' ', 'M', 's', 'g', 'I', 'D', '='};
    };

    inline uint8_t ExtendedHeader::get_verbosity() const noexcept { return verbose; }
    inline const char *ExtendedHeader::get_verbosity_cstr() const noexcept { return verbose ? verb_arr.data() : non_verb_arr.data(); }
    inline std::string ExtendedHeader::get_verbosity_str() const noexcept { return verbose ? std::string{verb_arr.data(), verb_arr.size()} : std::string{non_verb_arr.data(), non_verb_arr.size()}; }
    inline std::string_view ExtendedHeader::get_verbosity_view() const noexcept { return verbose ? std::string_view{verb_arr.data(), verb_arr.size()} : std::string_view{non_verb_arr.data(), non_verb_arr.size()}; }

    inline uint8_t ExtendedHeader::get_msg_type() const noexcept { return msg_type; }
    inline const char *ExtendedHeader::get_msg_type_cstr() const noexcept { return mstp_map.find(msg_type) != mstp_map.end() ? mstp_map.at(msg_type).data() : null_arr.data(); }
    inline std::string ExtendedHeader::get_msg_type_str() const noexcept { return mstp_map.find(msg_type) != mstp_map.end() ? std::string{mstp_map.at(msg_type).data(), mstp_map.at(msg_type).size()} : std::string{null_arr.data(), null_arr.size()}; }
    inline std::string_view ExtendedHeader::get_msg_type_view() const noexcept { return mstp_map.find(msg_type) != mstp_map.end() ? mstp_map.at(msg_type) : std::string_view{null_arr.data(), null_arr.size()}; }

    inline uint8_t ExtendedHeader::get_msg_type_info() const noexcept { return msg_type_info; }
    inline const char *ExtendedHeader::get_msg_type_info_cstr() const noexcept { return (mstp_map.find(msg_type) != mstp_map.end() && mstp_mtin_map.at(msg_type).find(msg_type_info) != mstp_mtin_map.at(msg_type).end()) ? mstp_mtin_map.at(msg_type).at(msg_type_info).data() : null_arr.data(); }
    inline std::string ExtendedHeader::get_msg_type_info_str() const noexcept { return (mstp_map.find(msg_type) != mstp_map.end() && mstp_mtin_map.at(msg_type).find(msg_type_info) != mstp_mtin_map.at(msg_type).end()) ? std::string{mstp_mtin_map.at(msg_type).at(msg_type_info).data(), mstp_mtin_map.at(msg_type).at(msg_type_info).size()} : std::string{null_arr.data(), null_arr.size()}; }
    inline std::string_view ExtendedHeader::get_msg_type_info_view() const noexcept { return (mstp_map.find(msg_type) != mstp_map.end() && mstp_mtin_map.at(msg_type).find(msg_type_info) != mstp_mtin_map.at(msg_type).end()) ? mstp_mtin_map.at(msg_type).at(msg_type_info) : std::string_view{null_arr.data(), null_arr.size()}; }

    inline uint8_t ExtendedHeader::get_num_args() const noexcept { return num_args; }
    inline uint32_t ExtendedHeader::get_non_verb_msg_id() const noexcept { return non_verb_msg_id; }

    inline const char *ExtendedHeader::get_apid_cstr() const noexcept { return application_id.data(); }
    inline std::string ExtendedHeader::get_apid_str() const noexcept { return {application_id.data(), application_id.size()}; }
    inline std::string_view ExtendedHeader::get_apid_view() const noexcept { return {application_id.data(), application_id.size()}; }
    inline const std::array<char, 4> &ExtendedHeader::get_apid_arr() const noexcept { return application_id; }

    inline const char *ExtendedHeader::get_ctid_cstr() const noexcept { return context_id.data(); }
    inline std::string ExtendedHeader::get_ctid_str() const noexcept { return {context_id.data(), context_id.size()}; }
    inline std::string_view ExtendedHeader::get_ctid_view() const noexcept { return {context_id.data(), context_id.size()}; }
    inline const std::array<char, 4> &ExtendedHeader::get_ctid_arr() const noexcept { return context_id; }

    void inline ExtendedHeader::append_extended_header_str(std::vector<char> &buf) const noexcept
    {
        // Append Apid to header string
        append_str_fixed(buf, apid_arr);
        auto apid_it = std::find(application_id.begin(), application_id.end(), '\0');
        append_array_partly(buf, application_id, apid_it);

        // Append Ctid to header string
        append_str_fixed(buf, ctid_arr);
        auto ctid_it = std::find(context_id.begin(), context_id.end(), '\0');
        append_array_partly(buf, context_id, ctid_it);

        // Append message type to header string
        append_str_fixed(buf, type_arr);
        if (mstp_map.find(msg_type) != mstp_map.end())
        {
            buf.insert(buf.end(), mstp_map.at(msg_type).begin(), mstp_map.at(msg_type).end());
        }
        else
        {
            append_str_fixed(buf, null_arr);
        }

        // Append message type info to header string
        append_str_fixed(buf, subtype_arr);
        if (mstp_map.find(msg_type) != mstp_map.end() && mstp_mtin_map.at(msg_type).find(msg_type_info) != mstp_mtin_map.at(msg_type).end())
        {
            buf.insert(buf.end(), mstp_mtin_map.at(msg_type).at(msg_type_info).begin(), mstp_mtin_map.at(msg_type).at(msg_type_info).end());
        }
        else
        {
            append_str_fixed(buf, null_arr);
        }

        // Append verbose and msg id to header string
        append_str_fixed(buf, mode_arr);
        if (verbose)
        {
            append_str_fixed(buf, verb_arr);
            append_str_fixed(buf, msgid_arr_0);
        }
        else
        {
            append_str_fixed(buf, non_verb_arr);
            append_str_fixed(buf, msgid_arr);
            append_number<uint32_t>(buf, non_verb_msg_id);
        }
    }

    /// @brief The Header object exists for each message. It stores the StorageHeader and ExtendedHeader and additional information of the message.
    class Header
    {

        // Interface
    public:
        Header() = delete;
        ~Header() = default;

        /// @brief Make the Header. This is only needed during parsing. There should be no need to create a Header by yourself.
        /// @param msg_idx The index of the message
        /// @param stohe The storage header
        /// @param htyp The header type
        /// @param mcnt The message count
        /// @param len The length of the message in bytes
        /// @param seid The session id
        /// @param tmsp The timestamp
        /// @param ecid The ecu id
        Header(std::streamsize msg_idx, StorageHeader stohe, uint8_t htyp, uint8_t mcnt, uint16_t len, uint32_t seid, float tmsp, std::array<char, 4> ecid) noexcept : msg_idx(msg_idx), storage_header(stohe), header_type(htyp), msg_count(mcnt), msg_len(len), ecu_id(ecid), session_id(seid), msg_timestamp(tmsp) {}

        // Make copy constructor
        Header(const Header &) noexcept = default;
        Header &operator=(const Header &) noexcept = default;

        // Make default move constructor
        Header(Header &&) noexcept = default;
        Header &operator=(Header &&) noexcept = default;

        /// @return The raw header type
        [[nodiscard]] uint8_t get_header_type() const noexcept;

        /// @return The message count
        [[nodiscard]] uint8_t get_msg_count() const noexcept;

        /// @return The message length in bytes
        [[nodiscard]] uint16_t get_msg_len() const noexcept;

        /// @return Whether the standard header includes an ECU ID (HTYP.WEID).
        [[nodiscard]] bool weid_present() const noexcept;

        /// @return The human readable ecu id c-string
        [[nodiscard]] const char *get_ecu_id_cstr() const noexcept;

        /// @return The human readable ecu id string
        [[nodiscard]] std::string get_ecu_id_str() const noexcept;

        /// @return The human readable ecu id as view
        [[nodiscard]] std::string_view get_ecu_id_view() const noexcept;

        /// @return The human readable ecu id as char array
        [[nodiscard]] const std::array<char, 4> &get_ecu_id_arr() const noexcept;

        /// @return The session id
        [[nodiscard]] uint32_t get_session_id() const noexcept;

        /// @return The entire human readable header as view
        [[nodiscard]] std::string_view to_view() noexcept;

        /// @return The entire human readable header string
        [[nodiscard]] std::string to_string() noexcept;

        /// @return The message index
        [[nodiscard]] std::streamsize get_msg_idx() const noexcept;

        // Class routines
    private:
        void make_header() noexcept;

        // Public member
    public:
        std::streamsize msg_idx;
        StorageHeader storage_header;
        uint8_t header_type;
        uint8_t msg_count;
        uint16_t msg_len;
        std::array<char, 4> ecu_id;
        uint32_t session_id;

        // Multiply by 0.0001f if printed to string
        float msg_timestamp;

        std::vector<char> str;
        ExtendedHeader extended_header;

        // Public constants
    public:
        static constexpr uint8_t STAHE_HTYP_BYTES = 0x01;
        static constexpr uint8_t STAHE_HTYP_UEH = 0x01;
        static constexpr uint8_t STAHE_HTYP_MSBF = 0x02;
        static constexpr uint8_t STAHE_HTYP_WEID = 0x04;
        static constexpr uint8_t STAHE_HTYP_WSID = 0x08;
        static constexpr uint8_t STAHE_HTYP_WTMS = 0x10;
        static constexpr uint8_t STAHE_HTYP_VERS = 0xE0;
        static constexpr uint8_t STAHE_HTYP_VERS_SHFT = 0x05;

        static constexpr uint8_t STAHE_MCNT_BYTES = 0x01;
        static constexpr int64_t STAHE_LEN_BYTES = 0x0000000000000002;
        static constexpr int64_t STAHE_ECUID_BYTES = 0x0000000000000004;
        static constexpr int64_t STAHE_SEID_BYTES = 0x0000000000000004;
        static constexpr int64_t STAHE_TMSP_BYTES = 0x0000000000000004;

        // Private constants
    private:
        static constexpr std::array<char, 6> index_arr = {'I', 'n', 'd', 'e', 'x', '='};
        static constexpr std::array<char, 12> tmsp_arr = {',', ' ', 'T', 'i', 'm', 'e', 's', 't', 'a', 'm', 'p', '='};
        static constexpr std::array<char, 8> cnt_arr = {',', ' ', 'C', 'o', 'u', 'n', 't', '='};
        static constexpr std::array<char, 8> ecuid_arr = {',', ' ', 'E', 'c', 'u', 'i', 'd', '='};
        static constexpr std::array<char, 12> ssid_arr = {',', ' ', 'S', 'e', 's', 's', 'i', 'o', 'n', 'I', 'd', '='};
    };

    inline uint8_t Header::get_header_type() const noexcept { return header_type; }
    inline uint8_t Header::get_msg_count() const noexcept { return msg_count; }
    inline uint16_t Header::get_msg_len() const noexcept { return msg_len; }
    inline bool Header::weid_present() const noexcept { return (header_type & STAHE_HTYP_WEID) != 0; }

    inline const char *Header::get_ecu_id_cstr() const noexcept { return ecu_id.data(); }
    inline std::string Header::get_ecu_id_str() const noexcept { return {ecu_id.data(), ecu_id.size()}; }
    inline std::string_view Header::get_ecu_id_view() const noexcept { return {ecu_id.data(), ecu_id.size()}; }
    inline const std::array<char, 4> &Header::get_ecu_id_arr() const noexcept { return ecu_id; }

    inline uint32_t Header::get_session_id() const noexcept { return session_id; }

    inline std::string Header::to_string() noexcept
    {
        if (str.empty())
        {
            make_header();
        }
        return {str.data(), str.size()};
    }
    inline std::string_view Header::to_view() noexcept
    {
        if (str.empty())
        {
            make_header();
        }
        return {str.data(), str.size()};
    }

    inline std::streamsize Header::get_msg_idx() const noexcept { return msg_idx; }

    inline void Header::make_header() noexcept
    {
        append_str_fixed(str, index_arr);
        append_number<long long>(str, msg_idx);

        storage_header.append_storage_header_str(str);

        // Append timestamp to header string
        append_str_fixed(str, tmsp_arr);
        append_number<float>(str, msg_timestamp * 0.0001f);

        // Append message count to header string
        append_str_fixed(str, cnt_arr);
        append_uint8(str, msg_count);

        // Append Ecuid to header string
        append_str_fixed(str, ecuid_arr);
        auto ecuid_it = std::find(ecu_id.begin(), ecu_id.end(), '\0');
        append_array_partly(str, ecu_id, ecuid_it);

        // Append session id to header string
        append_str_fixed(str, ssid_arr);
        append_number<uint32_t>(str, session_id);

        extended_header.append_extended_header_str(str);
    }

    inline std::array<char, 4> to_ecu_id(const char *msg_buffer, const std::streamsize &offset) noexcept
    {
        std::array<char, 4> arr{};
        std::copy_n(msg_buffer + offset, 4, arr.begin());
        return arr;
    }

} // namespace dltp

#endif // DLTPARSER_DLTHEADER_H