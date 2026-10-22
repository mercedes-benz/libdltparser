# SPDX-License-Identifier: MIT 
# Copyright (c) 2026 MBition GmbH

cmake_minimum_required(VERSION 3.15...3.28)
project(dltparser VERSION 1.0.0 LANGUAGES CXX)

# This library uses the C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Check and print the selected build type
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug")
endif()
message(STATUS "Selected build type: ${CMAKE_BUILD_TYPE}")

option(PYTHON_BINDINGS "Enable Python bindings support" OFF)
message(STATUS "PYTHON_BINDINGS=${PYTHON_BINDINGS}")

#set(pybind11_DIR "C:\\Program Files\\Python311\\Lib\\site-packages\\pybind11\\share\\cmake\\pybind11")
if(PYTHON_BINDINGS)
    list(APPEND SRC_FILES src/dltparser_bindings.cpp)
    find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)
    find_package(pybind11 CONFIG REQUIRED)
    include_directories(${pybind11_INCLUDE_DIRS})
endif()

# Include GoogleTest (https://github.com/google/googletest)
# Use Release v1.17.0 with commit 52eb810
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/52eb8108c5bdec04579160ae17225d66034bd723.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Create target
set(SRC_FILES 
    src/dltparser.cpp
    include/dltparser/utils.h
    include/dltparser/dltparser.h
    include/dltparser/dltsettings.h
    include/dltparser/dltheader.h
    include/dltparser/dltpayload.h
    include/dltparser/file_loader.h
    include/ctre/ctre.hpp
    include/ctre/ctre-unicode.hpp
    include/ctre/unicode-db.hpp
)

if(PYTHON_BINDINGS)
    list(APPEND SRC_FILES src/dltparser_bindings.cpp)
endif()

# Only build for python if not in Debug mode (due to Debug build issue)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Skipping the _core module in Debug build.")
else()
    if(PYTHON_BINDINGS)
        # Add _core module only for non-Debug builds
        python_add_library(_core MODULE ${SRC_FILES} WITH_SOABI)
        target_link_libraries(_core PRIVATE pybind11::headers)
        target_compile_features(_core PRIVATE cxx_std_17)
        target_compile_definitions(_core PRIVATE VERSION_INFO=1.0.0)
        install(TARGETS _core DESTINATION dltparser)
    endif()
endif()

add_library(dltparser ${SRC_FILES})

# Add alias so that the library can be used inside the build tree
add_library(DLTParser::dltparser ALIAS dltparser)

# Set target properties
target_include_directories(dltparser
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(dltparser PRIVATE cxx_std_17)
target_compile_options(dltparser PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
    -Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:
    /W4 /WX>
)

# Add Windows-specific compile definitions
if(WIN32)
    target_compile_options(dltparser PRIVATE
        $<$<CONFIG:Debug>:/Zi>
        $<$<CONFIG:Release>:/O2>
    )
endif()

target_compile_definitions(dltparser PRIVATE $<$<CONFIG:Debug>:DEBUG>)

# Set install paths for UNIX and WIN32
if(UNIX)
    include(GNUInstallDirs)
else()
    if(WIN32)
        if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
            set(CMAKE_INSTALL_PREFIX "C:/Program Files/libdltparser" CACHE PATH "Install path" FORCE)
        endif()
        set(CMAKE_INSTALL_LIBDIR "lib")
        set(CMAKE_INSTALL_DATADIR "share")
        set(CMAKE_INSTALL_INCLUDEDIR "include")
        set(CMAKE_INSTALL_BINDIR "bin")
    else()
        message(FATAL_ERROR "System not UNIX or WIN32 - not implemented yet")
    endif()
endif()
message(STATUS "Setting installation destination to: ${CMAKE_INSTALL_PREFIX}")

# Set place where the configs going to be installed
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/DLTParser)

install(TARGETS dltparser
    EXPORT dltparser-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/$<CONFIG>
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/$<CONFIG>
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/$<CONFIG>
)

# This is required so that the exported target has the name DLTParser and not dltparser
set_target_properties(dltparser PROPERTIES EXPORT_NAME DLTParser)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Export the targets to a script
install(EXPORT dltparser-targets
    FILE
        DLTParserTargets.cmake
    NAMESPACE
        DLTParser::
    DESTINATION
        ${INSTALL_CONFIGDIR}
)

# Create a ConfigVersion.cmake file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/DLTParserConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/DLTParserConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/DLTParserConfig.cmake
    INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
)

# Install the config, the configversion and the custom find modules
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/DLTParserConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/DLTParserConfigVersion.cmake
    DESTINATION ${INSTALL_CONFIGDIR}
)

# Export from the build tree
export(EXPORT dltparser-targets
    FILE ${CMAKE_CURRENT_BINARY_DIR}/DLTParserTargets.cmake
    NAMESPACE DLTParser::
)

export(PACKAGE DLTParser)

# From here: use gtest to build tests
enable_testing()

add_executable(Parser tests/dltparser_tests.cpp)
target_link_libraries(Parser GTest::gtest_main DLTParser::dltparser)

add_executable(Utility tests/utility_tests.cpp)
target_link_libraries(Utility GTest::gtest_main DLTParser::dltparser)

add_executable(Settings tests/dltsettings_tests.cpp)
target_link_libraries(Settings GTest::gtest_main DLTParser::dltparser)

add_executable(ParserDiagnostics tests/dltparser_diagnostic_tests.cpp)
target_link_libraries(ParserDiagnostics GTest::gtest_main DLTParser::dltparser)

include(GoogleTest)
gtest_discover_tests(Parser)
gtest_discover_tests(Utility)
gtest_discover_tests(Settings)
gtest_discover_tests(ParserDiagnostics)
