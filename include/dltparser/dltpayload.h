// SPDX-License-Identifier: MIT
// Copyright (c) 2026 MBition GmbH

#ifndef DLTPARSER_DLTPAYLOAD_H
#define DLTPARSER_DLTPAYLOAD_H

#include "utils.h"

namespace dltp
{

    /// @brief The Payload object exists for each message. It stores the entire parsed data which is present after the Header or ExtendedHeader of a message.
    class Payload
    {

        // Interface
    public:
        /// @brief Reflects the message type and therefore the Payload being verbose or non-verbose. If default, the Payload was not initialized yet.
        enum Type
        {
            DEFAULT,
            VERBOSE,
            NON_VERBOSE
        };

        /// @brief Make the Payload. This is only needed during parsing. There should be no need to create a Payload by yourself.
        /// @param msg_id The id of the message id if the message is non-verbose, otherwise 0
        /// @param type Either verbose of non-verbose
        Payload(uint32_t msg_id, Type type) noexcept : msg_id(msg_id), type(type) {}

        // Make copy constructor
        Payload(const Payload &) noexcept = default;
        Payload &operator=(const Payload &) noexcept = default;

        // Make default move constructor
        Payload(Payload &&) noexcept = default;
        Payload &operator=(Payload &&) noexcept = default;

        /// @return The message id, 0 for verbose, non 0 for non-verbose messages
        [[nodiscard]] uint32_t get_msg_id() const noexcept;

        /// @return The type of the message
        [[nodiscard]] Type get_type() const noexcept;

        /// @return The raw parsed characters
        [[nodiscard]] const std::vector<char> &get_data() const noexcept;

        /// @return The human readable payload string as view
        [[nodiscard]] std::string_view to_view() noexcept;

        /// @return The human readable payload string
        [[nodiscard]] std::string to_string() noexcept;

        // Class routines
    private:
        template <typename T, typename = std::enable_if_t<std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>>>
        T make_header() noexcept;

        // Public member
    public:
        uint32_t msg_id = 0;
        Type type = DEFAULT;
        std::vector<char> data;
        std::vector<char> str;
        uint32_t str_coding = 0;

        // Public constants
    public:
        static constexpr std::array<char, 4> BOOL_TRUE_STR_VAL = {'T', 'r', 'u', 'e'};
        static constexpr std::array<char, 5> BOOL_FALSE_STR_VAL = {'F', 'a', 'l', 's', 'e'};

        // Private constants
    private:
        // Services / Commands
        static constexpr uint32_t SRVC_SET_LOG_LEVEL = 0x01;
        static constexpr uint8_t SRVC_SET_LOG_LEVEL_BYTES = 0x0D;

        static constexpr uint32_t SRVC_SET_TRACE_STATUS = 0x02;
        static constexpr uint8_t SRVC_SET_TRACE_STATUS_BYTES = 0x0D;

        static constexpr uint32_t SRVC_GET_LOG_INFO = 0x03;
        static constexpr uint8_t SRVC_GET_LOG_INFO_BYTES = 0x0D;

        static constexpr uint32_t SRVC_GET_DEFAULT_LOG_LEVEL = 0x04;
        static constexpr uint8_t SRVC_GET_DEFAULT_LOG_LEVEL_BYTES = 0x00;

        static constexpr uint32_t SRVC_STORE_CONFIGURATION = 0x05;
        static constexpr uint8_t SRVC_STORE_CONFIGURATION_BYTES = 0x00;

        static constexpr uint32_t SRVC_RESTORE_TO_FACTORY_DEFAULT = 0x06;
        static constexpr uint8_t SRVC_RESTORE_TO_FACTORY_DEFAULT_BYTES = 0x00;

        static constexpr uint32_t SRVC_SET_COMMUNICATION_INTERFACE_STATUS = 0x07;
        static constexpr uint8_t SRVC_SET_COMMUNICATION_INTERFACE_STATUS_BYTES = 0x00;

        static constexpr uint32_t SRVC_SET_COMMUNICATION_INTERFACE_MAX_BANDWIDTH = 0x08;
        static constexpr uint8_t SRVC_SET_COMMUNICATION_INTERFACE_MAX_BANDWIDTH_BYTES = 0x00;

        static constexpr uint32_t SRVC_SET_VERBOSE_MODE = 0x09;
        static constexpr uint8_t SRVC_SET_VERBOSE_MODE_BYTES = 0x00;

        static constexpr uint32_t SRVC_SET_MESSAGE_FILTERING = 0x0A;
        static constexpr uint8_t SRVC_SET_MESSAGE_FILTERING_BYTES = 0x01;

        static constexpr uint32_t SRVC_SET_TIMING_PACKETS = 0x0B;
        static constexpr uint8_t SRVC_SET_TIMING_PACKETS_BYTES = 0x00;

        static constexpr uint32_t SRVC_GET_LOCAL_TIME = 0x0C;
        static constexpr uint8_t SRVC_GET_LOCAL_TIME_BYTES = 0x00;

        static constexpr uint32_t SRVC_USE_ECU_ID = 0x0D;
        static constexpr uint8_t SRVC_USE_ECU_ID_BYTES = 0x00;

        static constexpr uint32_t SRVC_USE_SESSION_ID = 0x0E;
        static constexpr uint8_t SRVC_USE_SESSION_ID_BYTES = 0x00;

        static constexpr uint32_t SRVC_USE_TIMESTAMP = 0x0F;
        static constexpr uint8_t SRVC_USE_TIMESTAMP_BYTES = 0x00;

        static constexpr uint32_t SRVC_USE_EXTENDED_HEADER = 0x10;
        static constexpr uint8_t SRVC_USE_EXTENDED_HEADER_BYTES = 0x10;

        static constexpr uint32_t SRVC_SET_DEFAULT_LOG_LEVEL = 0x11;
        static constexpr uint8_t SRVC_SET_DEFAULT_LOG_LEVEL_BYTES = 0x05;

        static constexpr uint32_t SRVC_SET_DEFAULT_TRACE_STATUS = 0x12;
        static constexpr uint8_t SRVC_SET_DEFAULT_TRACE_STATUS_BYTES = 0x05;

        static constexpr uint32_t SRVC_GET_SOFTWARE_VERSION = 0x13;
        static constexpr uint8_t SRVC_GET_SOFTWARE_VERSION_BYTES = 0x00;

        static constexpr uint32_t SRVC_MESSAGE_BUFFER_OVERFLOW = 0x14;
        static constexpr uint8_t SRVC_MESSAGE_BUFFER_OVERFLOW_BYTES = 0x00;

        static constexpr uint32_t SRVC_GET_DEFAULT_TRACE_STATUS = 0x15;
        static constexpr uint8_t SRVC_GET_DEFAULT_TRACE_STATUS_BYTES = 0x00;

        static constexpr uint32_t SRVC_GET_LOG_CHANNEL_NAMES = 0x17;
        static constexpr uint8_t SRVC_GET_LOG_CHANNEL_NAMES_BYTES = 0x00;

        static constexpr uint32_t SRVC_GET_TRACE_STATUS = 0x1F;
        static constexpr uint8_t SRVC_GET_TRACE_STATUS_BYTES = 0x08;

        static constexpr uint32_t SRVC_SET_LOG_CHANNEL_ASSIGNMENT = 0x20;
        static constexpr uint8_t SRVC_SET_LOG_CHANNEL_ASSIGNMENT_BYTES = 0x0D;

        static constexpr uint32_t SRVC_SET_LOG_CHANNEL_THRESHOLD = 0x21;
        static constexpr uint8_t SRVC_SET_LOG_CHANNEL_THRESHOLD_BYTES = 0x06;

        static constexpr uint32_t SRVC_GET_LOG_CHANNEL_THRESHOLD = 0x22;
        static constexpr uint8_t SRVC_GET_LOG_CHANNEL_THRESHOLD_BYTES = 0x04;

        static constexpr uint32_t SRVC_BUFFER_OVERFLOW_NOTIFICATION = 0x23;
        static constexpr uint8_t SRVC_BUFFER_OVERFLOW_NOTIFICATION_BYTES = 0x00;

        static constexpr uint32_t SRVC_SYNC_TIME_STAMP = 0x24;
        static constexpr uint8_t SRVC_SYNC_TIME_STAMP_BYTES = 0x00;

        static constexpr uint32_t SRVC_UNREGISTER_CONTEXT = 0x0F01;
        static constexpr uint8_t SRVC_UNREGISTER_CONTEXT_BYTES = 0x0C;

        static constexpr uint32_t SRVC_CONNECTION_INFO = 0x0F02;
        static constexpr uint8_t SRVC_CONNECTION_INFO_BYTES = 0x06;

        static constexpr uint32_t SRVC_TIMEZONE = 0x0F03;
        static constexpr uint8_t SRVC_TIMEZONE_BYTES = 0x06;

        static constexpr uint32_t SRVC_MARKER = 0x0F04;
        static constexpr uint8_t SRVC_MARKER_BYTES = 0x01;

        static constexpr uint32_t SRVC_CALLSW_CINJECTION = 0xFFF;
        static constexpr uint8_t SRVC_CALLSW_CINJECTION_BYTES = 0x00;

        static inline const std::unordered_map<uint32_t, std::string_view> srvc_map{
            {SRVC_SET_LOG_LEVEL, "[set_log_level]"},
            {SRVC_SET_TRACE_STATUS, "[set_trace_status]"},
            {SRVC_GET_LOG_INFO, "[get_log_info]"},
            {SRVC_GET_DEFAULT_LOG_LEVEL, "[get_default_log_level]"},
            {SRVC_STORE_CONFIGURATION, "[store_configuration]"},
            {SRVC_RESTORE_TO_FACTORY_DEFAULT, "[restore_to_factory_default]"},
            {SRVC_SET_COMMUNICATION_INTERFACE_STATUS, "[set_communication_interface_status]"},
            {SRVC_SET_COMMUNICATION_INTERFACE_MAX_BANDWIDTH, "[set_communication_interface_max_bandwidth]"},
            {SRVC_SET_VERBOSE_MODE, "[set_verbose_mode]"},
            {SRVC_SET_MESSAGE_FILTERING, "[set_message_filtering]"},
            {SRVC_SET_TIMING_PACKETS, "[set_timing_packets]"},
            {SRVC_GET_LOCAL_TIME, "[get_local_time]"},
            {SRVC_USE_ECU_ID, "[use_ecu_id]"},
            {SRVC_USE_SESSION_ID, "[use_session_id]"},
            {SRVC_USE_TIMESTAMP, "[use_timestamp]"},
            {SRVC_USE_EXTENDED_HEADER, "[use_extended_header]"},
            {SRVC_SET_DEFAULT_LOG_LEVEL, "[set_default_log_level]"},
            {SRVC_SET_DEFAULT_TRACE_STATUS, "[set_default_trace_status]"},
            {SRVC_GET_SOFTWARE_VERSION, "[get_software_version]"},
            {SRVC_MESSAGE_BUFFER_OVERFLOW, "[message_buffer_overflow]"},
            {SRVC_GET_DEFAULT_TRACE_STATUS, "[get_default_trace_status]"},
            {SRVC_GET_LOG_CHANNEL_NAMES, "[get_log_channel_names]"},
            {SRVC_SET_LOG_CHANNEL_ASSIGNMENT, "[get_log_channel_assignments]"},
            {SRVC_SET_LOG_CHANNEL_THRESHOLD, "[set_log_channel_threshold]"},
            {SRVC_GET_LOG_CHANNEL_THRESHOLD, "[get_log_channel_threshold]"},
            {SRVC_BUFFER_OVERFLOW_NOTIFICATION, "[buffer_overflow_notification]"},
            {SRVC_SYNC_TIME_STAMP, "[sync_timestamp]"},
            {SRVC_UNREGISTER_CONTEXT, "[unregister_context]"},
            {SRVC_CONNECTION_INFO, "[connection_info]"},
            {SRVC_TIMEZONE, "[timezone]"},
            {SRVC_MARKER, "[marker]"},
            {SRVC_CALLSW_CINJECTION, "[callsw_cinjection]"},
            {SRVC_GET_TRACE_STATUS, "[get_trace_status]"}};

        static inline const std::unordered_map<uint32_t, uint8_t> srvc_bytes_map{
            {SRVC_SET_LOG_LEVEL, SRVC_SET_LOG_LEVEL_BYTES},
            {SRVC_SET_TRACE_STATUS, SRVC_SET_TRACE_STATUS_BYTES},
            {SRVC_GET_LOG_INFO, SRVC_GET_LOG_INFO_BYTES},
            {SRVC_GET_DEFAULT_LOG_LEVEL, SRVC_GET_DEFAULT_LOG_LEVEL_BYTES},
            {SRVC_STORE_CONFIGURATION, SRVC_STORE_CONFIGURATION_BYTES},
            {SRVC_RESTORE_TO_FACTORY_DEFAULT, SRVC_RESTORE_TO_FACTORY_DEFAULT_BYTES},
            {SRVC_SET_COMMUNICATION_INTERFACE_STATUS, SRVC_SET_COMMUNICATION_INTERFACE_STATUS_BYTES},
            {SRVC_SET_COMMUNICATION_INTERFACE_MAX_BANDWIDTH, SRVC_SET_COMMUNICATION_INTERFACE_MAX_BANDWIDTH_BYTES},
            {SRVC_SET_VERBOSE_MODE, SRVC_SET_VERBOSE_MODE_BYTES},
            {SRVC_SET_MESSAGE_FILTERING, SRVC_SET_MESSAGE_FILTERING_BYTES},
            {SRVC_SET_TIMING_PACKETS, SRVC_SET_TIMING_PACKETS_BYTES},
            {SRVC_GET_LOCAL_TIME, SRVC_GET_LOCAL_TIME_BYTES},
            {SRVC_USE_ECU_ID, SRVC_USE_ECU_ID_BYTES},
            {SRVC_USE_SESSION_ID, SRVC_USE_SESSION_ID_BYTES},
            {SRVC_USE_TIMESTAMP, SRVC_USE_TIMESTAMP_BYTES},
            {SRVC_USE_EXTENDED_HEADER, SRVC_USE_EXTENDED_HEADER_BYTES},
            {SRVC_SET_DEFAULT_LOG_LEVEL, SRVC_SET_DEFAULT_LOG_LEVEL_BYTES},
            {SRVC_SET_DEFAULT_TRACE_STATUS, SRVC_SET_DEFAULT_TRACE_STATUS_BYTES},
            {SRVC_GET_SOFTWARE_VERSION, SRVC_GET_SOFTWARE_VERSION_BYTES},
            {SRVC_MESSAGE_BUFFER_OVERFLOW, SRVC_MESSAGE_BUFFER_OVERFLOW_BYTES},
            {SRVC_GET_DEFAULT_TRACE_STATUS, SRVC_GET_DEFAULT_TRACE_STATUS_BYTES},
            {SRVC_GET_LOG_CHANNEL_NAMES, SRVC_GET_LOG_CHANNEL_NAMES_BYTES},
            {SRVC_SET_LOG_CHANNEL_ASSIGNMENT, SRVC_SET_LOG_CHANNEL_ASSIGNMENT_BYTES},
            {SRVC_SET_LOG_CHANNEL_THRESHOLD, SRVC_SET_LOG_CHANNEL_THRESHOLD_BYTES},
            {SRVC_GET_LOG_CHANNEL_THRESHOLD, SRVC_GET_LOG_CHANNEL_THRESHOLD_BYTES},
            {SRVC_BUFFER_OVERFLOW_NOTIFICATION, SRVC_BUFFER_OVERFLOW_NOTIFICATION_BYTES},
            {SRVC_SYNC_TIME_STAMP, SRVC_SYNC_TIME_STAMP_BYTES},
            {SRVC_UNREGISTER_CONTEXT, SRVC_UNREGISTER_CONTEXT_BYTES},
            {SRVC_CONNECTION_INFO, SRVC_CONNECTION_INFO_BYTES},
            {SRVC_TIMEZONE, SRVC_TIMEZONE_BYTES},
            {SRVC_MARKER, SRVC_MARKER_BYTES},
            {SRVC_CALLSW_CINJECTION, SRVC_CALLSW_CINJECTION_BYTES},
            {SRVC_GET_TRACE_STATUS, SRVC_GET_TRACE_STATUS_BYTES}};
    };

    inline uint32_t Payload::get_msg_id() const noexcept { return msg_id; }
    inline Payload::Type Payload::get_type() const noexcept { return type; }
    inline const std::vector<char> &Payload::get_data() const noexcept { return data; }

    inline std::string Payload::to_string() noexcept { return make_header<std::string>(); }
    inline std::string_view Payload::to_view() noexcept { return make_header<std::string_view>(); }

    template <typename T, typename>
    T Payload::make_header() noexcept
    {
        data.erase(std::remove(data.begin(), data.end(), '\n'), data.end());

        if (type == VERBOSE)
        {
            return {data.data(), data.size()};
        }

        if (type == NON_VERBOSE)
        {
            if (str.empty())
            {
                if (srvc_map.find(msg_id) != srvc_map.end())
                {
                    str.insert(str.end(), srvc_map.at(msg_id).begin(), srvc_map.at(msg_id).end());
                    str.emplace_back(' ');
                    appends_str_hex(str, data.data(), static_cast<std::streamsize>(data.size()));
                    return {str.data(), str.size()};
                }

                str.emplace_back('[');
                str.emplace_back(']');
                appends_str_hex(str, data.data(), static_cast<std::streamsize>(data.size()));
                return {str.data(), str.size()};
            }

            return {str.data(), str.size()};
        }

        return {};
    }

} // namespace dltp

#endif // DLTPARSER_DLTPAYLOAD_H