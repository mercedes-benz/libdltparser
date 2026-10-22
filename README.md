# DLTParser

### A simple DLT (Diagnostic Log and Trace) parser for the [AUTOSAR R19-11](https://www.autosar.org/fileadmin/standards/R19-11/FO/AUTOSAR_PRS_LogAndTraceProtocol.pdf) specification.

---

**Note:** This parser does not support rendering verbose DLT arguments of array, struct, 16-bit/128-bit floating-point, or 128-bit signed/unsigned integer types. Messages containing these argument types may not be parsed correctly; subsequent arguments or messages can be affected.
**We are interested in all DLT files which could not be parsed or crashed this library! Please submit them opening an issue.**

**Note**: The initial version of this library was written by Simon Andreas Piorecki during his time at the MBition GmbH.

**Note:**: This project is licensed under MIT license, except the files inside include/ctre that are from ctre project (GitHub repo: https://github.com/hanickadot/compile-time-regular-expressions ) and are licensed under Apache-2.0.

---

### Overview

This DLT parser is supposed to parse DLT files fast and search for certain substrings or regex search/matches within the payload string. You can also get single messages and their Header/Payload.
Speed is the major requirement for this library.

#### Error Handling

The parser reports recoverable issues such as incomplete buffer fragments, corrupt carry-over data, and invalid non-verbose payload lengths through an optional C++ diagnostic callback. If a message cannot be parsed, it tries to recover and continue with the next valid message.
Please check below on how you are supposed to use this library!

#### Features
- Fast DLT file parsing
- Substring search within payload string
- Regex search/match within payload string

#### Missing Features
- More comprehensive tests for Python bindings
- More unit tests for C++ functions

### Installation

Necessary dependencies for building yourself are:
- Valid compiler (MSVC, Clang, gcc)
- Pybind11

#### (Recommended) Wheel, for use with Python:

- Determine your Python version
    - Open a terminal and execute `python3 --version`
    - It will show something like `Python 3.11.4`; the corresponding major/minor version is 3.11
- Download the correct wheel
    - `dltparser-1.0.0-cp312-cp312-win_amd64.whl` is an example wheel filename
    - `cp312` refers to Python 3.12
    - Choose `cp311` for Python 3.11, `cp312` for Python 3.12, and so on
    - Always choose the wheel with the ending `..._amd64` if you are running a 64bit Windows system
- Install the wheel
    - Use `pip install <path_to_wheel>` to install it
    - Example: `python3 -m pip install dltparser-1.0.0-cp312-cp312-win_amd64.whl`

#### Build and install the library, for us with C++: MSVC 2022 or GCC

- Clone the repository, then
  - `cd DLTParser`
  - `mkdir build`
  - `cd build`

- Build for either Debug or Release
- Windows
  - Open the MSVC developer console with admin privileges
  - Set to x64 environment `"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64`
  - `cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug -Dpybind11_DIR="C:/Program Files/.../cmake/pybind11" ..` (for Debug)
  - `cmake --build . --config Debug --target install`
  - *Or*
  - `cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release -Dpybind11_DIR="C:/Program Files/.../cmake/pybind11" ..` (for Release)
  - `cmake --build . --config Release --target install`
- Linux
  - `cmake -DCMAKE_BUILD_TYPE=Debug -Dpybind11_DIR="/.../cmake/pybind11" ..` (for Debug)
  - `sudo cmake --build . --target install` (required only when the installation prefix requires elevated permissions)
  - *Or*
  - `cmake -DCMAKE_BUILD_TYPE=Debug -Dpybind11_DIR="/.../cmake/pybind11" ..` (for Release)
  - `sudo cmake --build . --target install` (required only when the installation prefix requires elevated permissions)

Will install files under the default path `C:/Program Files/libdltparser/` on Windows or `/usr/local/...` on Linux.
You can set the installation path with `-DCMAKE_INSTALL_PREFIX="/.../"` when generating cmake files

#### Linking with CMake

`find_package(DLTParser REQUIRED)` \
`...` \
`target_link_libraries(YourCoolProject PRIVATE DLTParser::DLTParser)`

#### C++: When build your application together with DLTParser library

`cmake -G "Visual Studio 17 2022" -A x64 .. -DCMAKE_BUILD_TYPE=Debug -DDLTParser_DIR=C:\\Program Files\\libdltparser\\lib\\cmake\\DLTParser` \
for Debug build or \
`cmake -G "Visual Studio 17 2022" -A x64 .. -DCMAKE_BUILD_TYPE=Release -DDLTParser_DIR=C:\\Program Files\\libdltparser\\lib\\cmake\\DLTParser` \
for Release build

Note: You can also set the library path directly into your CMakeLists.txt of your project: `set(DLTParser_DIR C:\\Program Files\\libdltparser\\lib\\cmake\\DLTParser)`

### Usage

Note: doc/documentation.html is redirecting to the doxygen documentation.

The main interface is the DLTFile class. Provide the path to the DLT file to parse it; absolute and relative paths are supported.
If you add the optional settings object, you can specify regex strings or substring strings to check for the existance of those within the payload string while parsing.

Note: Using regex search/match can increase parsing time significantly.

#### Basic usage

Generally, you do the following:
- Create a Settings object
  - Set optional parameter like substring strings or regex search/match strings
- Create a DLTFile object passing the file path and the previously created Settings object
- Call the parse() function
- Get the messages or/and hits by calling get_messages() or/and get_hits()
  - Get hits of your substring / regex search by using the search string as key of the map
- Each message is a pair of `dltparser.Header` and `dltparser.Payload`
  - Example: 
  ```
  msgs = my_dlt_file.get_messages()
  first_msg = msgs[0]
  first_msg_header = first_msg[0]
  first_msg_payload = first_msg[1]
  print(first_msg_header.to_string())
  print(first_msg_payload.to_string())
  ```

#### Python

Within python, you can use `help(dltparser)` or `help(dltparser.DLTFile)` etc. to see all available functions

```
import dltparser
import os

dlt_file_path = os.path.normpath("my_dlt_file.DLT")

# Create the settings for your parsing. Usually, you only need to set the list with substr_strs or re_strs
# msg_buf_res_count: Optional, Default=2000000
#     Specifies the amount of messages to be allocated. It may speed up parsing.
# with_raw_in_hex: Optional, Default=False
#     Specifies if war byte message should be visible as hexadecimal string and therefore inclusded in the substring and regex lookup
# substr_strs: Optional, Default=[]
#     Specify one or more strings which are used for substring search within each message during parsing
# re_strs: Optional, Default=[]
#     Specify one or more strings which are used for regex search/match within each message during parsing. You need to specify the regex pattern as shown below.
my_dlt_settings = dltparser.Settings(msg_buf_res_count=2000000, with_raw_in_hex=False, substr_strs=["KPI_MARKER", "my_substr"], re_strs=[("my_regex1", dltparser.Settings.RePattern.REGEX_SEARCH), ("my_rgx2", dltparser.Settings.RePattern.REGEX_MATCH)])

# Loads the dlt file from disk
my_dlt_file = dltparser.DLTFile(filepath=dlt_file_path, settings=my_dlt_settings)
print(f'Loaded {my_dlt_file.size()} bytes!')

# Parses the dlt file with a given search string
if my_dlt_file.parse():
    print('Parsed successful!')

# Get all messages
msgs = my_dlt_file.get_messages()
print(len(msgs))

# Get all hits for "KPI_MARKER"
hits = my_dlt_file.get_hits()["KPI_MARKER"]
print(len(hits))
```


#### C++

Include the header file. This header describes all interface functions under ```public:``` within the ```DLTFile``` class.

```
#include "dltparser/dltparser.h"

std::string file_path{"my_dlt.DLT"};
dltp::Settings dlt_settings{0x00000000001E8480, false, true, {"KPI_MARKER"}};

// For regex search
// dltp::Settings dlt_settings{0x00000000001E8480, false, true, {"KPI_MARKER"}, {{"[\\s\\S]*KPI_MARKER[\\s\\S]*", dltp::Settings::RePattern::REGEX_MATCH}, {"KPI", dltp::Settings::RePattern::REGEX_SEARCH}}};

dltp::DLTFile dlt_file(file_path, dlt_settings);
dlt_file.parse();

auto hit_vec = dlt_file.get_hits()["KPI_MARKER"];
for (auto& [h, p] : hit_vec) {
  std::cout << p.to_string() << std::endl;
}

```

### Build and deploy Python wheels

Making wheels can be tricky if not all tools are installed properly or errors occur.
Making wheels is done with `cibuildwheel`

- Clone the repository and: `cd DLTParser`
- To build all wheels: `cibuildwheel --output-dir wheelhouse`

#### Windows:

(This will probably fail due to environment issues of both x86 and amd64)
    - Open the MSVC2022 developer console as administrator
    - Set the architecture for cibuildwheel with `set CIBW_ARCHS=x86` for 32-bit or `set CIBW_ARCHS=AMD64` for 64-bit
    - Set the console architecture with (x64) `"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64`
    - Run `cibuildwheel --output-dir wheelhouse`

This can fail due to multiple issues. You need to have installed all the python dependencies and MSVC 17 2022.
To make sure, compiling in general is possible, you should first compile it manually to a static library as shown above.
If this works, trying to make the wheels is the next step.
Following are issues I faced and could resolve.

- You maybe need to download nuget.exe manually and copy it to the correct path
    - Download [nuget](https://learn.microsoft.com/de-de/nuget/install-nuget-client-tools?tabs=windows)
    - Move it to the python lib cache, for example to: `C:\Users\<YOUR_USER>\AppData\Local\pypa\cibuildwheel\Cache`

#### Linux:

Not tested yet.
