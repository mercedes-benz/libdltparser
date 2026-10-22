// SPDX-License-Identifier: MIT
// Copyright (c) 2026 MBition GmbH

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <filesystem>
#include "../include/dltparser/dltparser.h"
#include "../include/dltparser/dltheader.h"
#include "../include/dltparser/dltpayload.h"
#include "../include/dltparser/dltsettings.h"

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;

PYBIND11_MODULE(_core, m)
{
    m.doc() = R"pbdoc(
        Pybind11 dltparser module.
        -----------------------

        Please use help() to find information about the usage of this module.
        For example: help(dltparser) or help(dltparser.Settings) or help(dltparser.DLTFile).
        
        You can also have a look into the documentation.html within the main repository.

    )pbdoc";

    // Settings class declaration
    py::class_<dltp::Settings> pySettings(m, "Settings",
                                          R"pbdoc(
        dltparser.Settings object that defines how payloads are parsed and filtered.

        You can:
        - Specify how many messages are expected (to preallocate memory)
        - Decide whether raw byte messages should be hex-encoded
        - Decide whether messages are saved within the DLTFile object or not
        - Define substrings and regular expressions to search or match against message payloads

    )pbdoc");

    // Nested RePattern enum class
    py::enum_<dltp::Settings::RePattern>(pySettings, "RePattern",
                                         R"pbdoc(
        Enum specifying how regex patterns are applied.

        - `REGEX_SEARCH`: Use `std::regex_search`, which looks for the pattern *anywhere* in the string.
        - `REGEX_MATCH`: Use `std::regex_match`, which requires the *entire* string to match.
    )pbdoc")
        .value("REGEX_MATCH", dltp::Settings::RePattern::REGEX_MATCH)
        .value("REGEX_SEARCH", dltp::Settings::RePattern::REGEX_SEARCH)
        .export_values();

    // Settings class definition
    pySettings
        .def(py::init([](std::size_t msg_buf_res_count, bool with_raw_in_hex, bool save_msgs, const std::vector<std::string> &substr_strs, const std::vector<std::pair<std::string, dltp::Settings::RePattern>> &re_strs)
                      { return dltp::Settings{msg_buf_res_count, with_raw_in_hex, save_msgs, substr_strs, re_strs}; }),
             py::kw_only(),
             py::arg("msg_buf_res_count") = 0x00000000001E8480,
             py::arg("with_raw_in_hex") = false,
             py::arg("save_msgs") = true,
             py::arg("substr_strs") = std::vector<std::string>{},
             py::arg("re_strs") = std::vector<std::pair<std::string, dltp::Settings::RePattern>>{},
             R"pbdoc(
            Constructor to create the dltparser.Settings object for parsing.

            Args:
                msg_buf_res_count Optional[int]: The amount of messages to be expected; the internal buffer will be reserved accordingly.
                with_raw_in_hex Optional[bool]: If true, raw byte payloads will be hex-encoded.
                save_msgs Optional[bool]: If true, messages are saved within the DLTFile object.
                substr_strs Optional[list[str]]: Substrings used for plain text search within payloads.
                re_strs Optional[list[tuple[str, RePattern]]]: Regular expressions to be used for payload filtering.

            Raises:
                regex_error: If a regex could not be created.
                MemoryError: If memory allocation fails (e.g. std::bad_alloc).

        )pbdoc")
        .def("with_substr", &dltp::Settings::with_substr, "Returns True if a substring search string was passed during object creation, otherwise False")
        .def("with_regex_search", &dltp::Settings::with_regex_search, "Returns True if a regex string with match pattern was passed during object creation, otherwise False")
        .def("with_regex_match", &dltp::Settings::with_regex_match, "Returns True if a regex string with match pattern was passed during object creation, otherwise False")

        .def("get_with_raw_in_hex", &dltp::Settings::get_with_raw_in_hex, "Returns True if with_raw_in_hex was passed as true during object creation, otherwise False")
        .def("get_msg_buffer_reserve_count", &dltp::Settings::get_msg_buffer_reserve_count, "Returns the amount which was passed for msg_buffer_reserve_count during object creation, otherwise 2 million")
        .def("get_substr_strs", &dltp::Settings::get_substr_strs, "Returns the substring search strings which were passed during object creation, otherwise an empty list")
        .def("get_search_regex_strs", &dltp::Settings::get_search_regex_strs, "Returns the regex strings with search pattern which were passed during object creation, otherwise an empty list")
        .def("get_match_regex_strs", &dltp::Settings::get_match_regex_strs, "Returns the regex strings with match pattern which were passed during object creation, otherwise an empty list");

    // Payload class declaration
    py::class_<dltp::Payload> pyPayload(m, "Payload",
                                        R"pbdoc(
        dltparser.Payload object exists for each message. It stores the entire parsed data which is present after the header or extended header of a message.
            
    )pbdoc");

    // Nested Type enum class
    py::enum_<dltp::Payload::Type>(pyPayload, "Type",
                                   R"pbdoc(
        Reflects the message type and therefore the payload being verbose or non-verbose. If default, the payload was not initialized yet.

        - `DEFAULT`: The payload was not initialized yet.
        - `VERBOSE`: A verbose message.
        - `NON_VERBOSE`: A non-verbose message.
    )pbdoc")
        .value("DEFAULT", dltp::Payload::Type::DEFAULT)
        .value("VERBOSE", dltp::Payload::Type::VERBOSE)
        .value("NON_VERBOSE", dltp::Payload::Type::NON_VERBOSE)
        .export_values();

    // Payload class definition
    pyPayload
        .def("get_msg_id", &dltp::Payload::get_msg_id, "Returns the message id, 0 for verbose, non 0 for non-verbose messages")
        .def("get_type", &dltp::Payload::get_type, "Returns the type of the message")
        .def("get_data", &dltp::Payload::get_data, "Returns the raw parsed characters")
        .def("to_string", &dltp::Payload::to_string, "Returns the human readable payload string");

    // StorageHeader class declaration and definition
    py::class_<dltp::StorageHeader>(m, "StorageHeader",
                                    R"pbdoc(
        The dltparser.StorageHeader object exists for each message. It stores the timestamp and the ecu id of the message.

    )pbdoc")
        .def("get_timestamp_str", &dltp::StorageHeader::get_timestamp_str, "Returns the human readable timestamp string")
        .def("get_timestamp_s", &dltp::StorageHeader::get_timestamp_s, "Returns the seconds of the timestamp")
        .def("get_timestamp_ms", &dltp::StorageHeader::get_timestamp_ms, "Returns the milliseconds of the timestamp")
        .def("get_ecu_id_str", &dltp::StorageHeader::get_ecu_id_str, "Returns the ecu id string");

    // ExtendedHeader class declaration and definition
    py::class_<dltp::ExtendedHeader>(m, "ExtendedHeader",
                                     R"pbdoc(
        The dltparser.ExtendedHeader object exists for message with an extended header. It stores extended information of the message.

    )pbdoc")
        .def("get_verbosity", &dltp::ExtendedHeader::get_verbosity, "Returns the raw verbosity")
        .def("get_verbosity_str", &dltp::ExtendedHeader::get_verbosity_str, "Returns the human readable verbosity string")
        .def("get_msg_type", &dltp::ExtendedHeader::get_msg_type, "Returns the raw message type")
        .def("get_msg_type_str", &dltp::ExtendedHeader::get_msg_type_str, "Returns the human readable message type string")
        .def("get_msg_type_info", &dltp::ExtendedHeader::get_msg_type_info, "Returns the raw message type info")
        .def("get_msg_type_info_str", &dltp::ExtendedHeader::get_msg_type_info_str, "Returns the human readable message type info string")
        .def("get_num_args", &dltp::ExtendedHeader::get_num_args, "Returns the number of arguments")
        .def("get_non_verb_msg_id", &dltp::ExtendedHeader::get_non_verb_msg_id, "Returns the message id of a non-verbose message")
        .def("get_apid_str", &dltp::ExtendedHeader::get_apid_str, "Returns the application id string")
        .def("get_ctid_str", &dltp::ExtendedHeader::get_ctid_str, "Returns the content id string");

    // Header class declaration and definition
    py::class_<dltp::Header>(m, "Header",
                             R"pbdoc(
        The dltparser.Header object exists for each message. It stores the storage and extended header and additional information of the message.

    )pbdoc")
        .def("get_header_type", &dltp::Header::get_header_type, "Returns the raw header type")
        .def("get_msg_count", &dltp::Header::get_msg_count, "Returns the message count")
        .def("get_msg_len", &dltp::Header::get_msg_len, "Returns the message length in bytes")
        .def("get_ecu_id_str", &dltp::Header::get_ecu_id_str, "Returns the human readable ecu id string")
        .def("get_session_id", &dltp::Header::get_session_id, "Returns the session id")
        .def("to_string", &dltp::Header::to_string, "Returns the entire human readable header string")
        .def("get_msg_idx", &dltp::Header::get_msg_idx, "Returns the message index");

    // DLTFile class declaration and definition
    py::class_<dltp::DLTFile<>>(m, "DLTFile",
                                R"pbdoc(
        The dltparser.DLTFile is the main object you are using to parse an AUTOSAR DLT file.
        It first loads the entire file into memory and then parses based on the given dltparser.Settings.
        After parsing, you can access all messages or hits based on your passed substring or regex strings within your dltparser.Settings.

    )pbdoc")
        .def(
            py::init<std::string>(),
            py::arg("filepath"),
            R"pbdoc(
            The simple default constructor to parse a DLT file without any substring or regex search.

            Args:
                filepath str: The absolute filepath to the DLT file

        )pbdoc")
        .def(
            py::init<std::string, dltp::Settings>(),
            py::kw_only(),
            py::arg("filepath"),
            py::arg("settings"),
            R"pbdoc(
            The standard constructor to parse a DLT file and search for substrings or regex within the message payloads.

            Args:
                filepath str: The absolute filepath to the DLT file
                settings dltparser.Settings: The settings object in which substrings, regex and other options are given for parsing

        )pbdoc")
        // TODO: Add callback constructor definition
        .def("parse", &dltp::DLTFile<>::parse, "Returns True if parsing finished successfully, otherwise False")
        .def("save_messages", &dltp::DLTFile<>::save_messages, py::arg("filepath"), "Returns True if saving was successful, otherwise False")
        .def("save_hits", &dltp::DLTFile<>::save_hits, py::arg("filepath"), "Returns True if saving was successful, otherwise False")
        .def("get_messages", &dltp::DLTFile<>::get_messages, pybind11::return_value_policy::reference, "Returns all messages as dltparser.Header, dltparser.Payload pair")
        .def("get_hits", &dltp::DLTFile<>::get_hits, pybind11::return_value_policy::reference, "Returns a dictionary with your substring or regex string as key and all your hits as dltparser.Header, dltparser.Payload pair as value")
        .def("is_read", &dltp::DLTFile<>::is_read, "Returns True if the DLT file was read into memory successfully, otherwise False")
        .def("size", &dltp::DLTFile<>::size, "The size of the DLT file in bytes");

#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}