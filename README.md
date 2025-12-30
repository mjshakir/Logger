# Logger

## Summary

The Logger library is a lightweight, fast, and efficient logging utility for C++ projects. It provides various log levels (debug, error, warning, normal), supports color-coded output, and can use either the `fmt` library (default) or the standard library (`std::format` / `std::print`) for formatting and output.

## Features

- Different log levels: Debug, Error, Warning, Normal
- Color-coded console output for errors and warnings
- Selectable formatting backend: `fmt` (default) or `std::format`
- Thread-safe logging with minimal overhead
- Support for writing logs to files with timestamps

## Installation

### Install dependencies:

#### Ubuntu

```bash
sudo apt-get update
```
```bash
sudo apt-get install -y cmake ninja-build libfmt-dev
```
#### macOS (using `Homebrew`)
```sh
brew update
```
```sh
brew install cmake ninja fmt
```
#### Windows
You can use `vcpkg` to install `fmt` on Windows (needed for the default `fmt` backend).
```sh
git clone https://github.com/microsoft/vcpkg.git
```
```sh
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
.\vcpkg install fmt
```
Ensure that `vcpkg` is integrated with your Visual Studio or CMake projects.

## Building the Project
1. **Clone the repository**
```bash
git clone https://github.com/mjshakir/Logger.git
```
```bash
cd Logger
```
2. **Run CMake to configure the project**
```bash
cmake -DFORCE_COLORED_OUTPUT=ON -DCMAKE_BUILD_TYPE=Release -B build -G Ninja
```
3. **Build the project:**
```bash
cd build/
```
```bash
ninja
```

## Formatting Backend Options

### Default: `fmt` backend (C++20)
`fmt` is the default backend. This is typically faster and also works on compilers/stdlibs that don't have full `std::format` support yet.

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
```

### Use `std::format` / `std::print` backend (C++23)
If your compiler + standard library support C++23 `<format>` and `<print>`, you can opt-in:

```bash
cmake -B build_std -G Ninja -DCMAKE_BUILD_TYPE=Release -DLOGGER_FORCE_FMT=OFF
```

### `std::format_string` (compile-time format checking)
When using the std backend, you can switch the logger APIs to `std::format_string<Args...>` and use `std::format` (instead of `std::vformat`):

```bash
cmake -B build_std_checked -G Ninja -DCMAKE_BUILD_TYPE=Release -DLOGGER_FORCE_FMT=OFF -DLOGGER_USE_STD_FORMAT_STRING=ON
```

If you need runtime format strings with the std backend, disable it:

```bash
cmake -B build_std_runtime -G Ninja -DCMAKE_BUILD_TYPE=Release -DLOGGER_FORCE_FMT=OFF -DLOGGER_USE_STD_FORMAT_STRING=OFF
```

## CMake Options

You can set options at configure time (standalone build) with `-D<option>=ON|OFF`, or before `add_subdirectory(...)` (when embedding Logger).

| Option | Default | What it does | How to set |
|---|---:|---|---|
| `LOGGER_FORCE_FMT` | `ON` | Forces the `fmt` backend even if `std::format` is available. Also selects the library language standard: `C++20` when `ON`, `C++23` when `OFF` (and `std::format` is detected). | `-DLOGGER_FORCE_FMT=ON`<br>`set(LOGGER_FORCE_FMT ON)` |
| `LOGGER_USE_STD_FORMAT_STRING` | `ON` | Std-backend only (`LOGGER_FORCE_FMT=OFF`): uses `std::format_string<Args...>` + `std::format` (better compile-time checking, can be faster). When `ON`, runtime format strings are not supported. | `-DLOGGER_USE_STD_FORMAT_STRING=ON`<br>`set(LOGGER_USE_STD_FORMAT_STRING ON)` |
| `BUILD_LOGGER_SHARED_LIBS` | `ON` | Build Logger as a shared library (`OFF` builds a static library). | `-DBUILD_LOGGER_SHARED_LIBS=OFF`<br>`set(BUILD_LOGGER_SHARED_LIBS OFF)` |
| `BUILD_LOGGER_EXAMPLE` | `ON` (standalone) | Build the example executable. | `-DBUILD_LOGGER_EXAMPLE=ON`<br>`set(BUILD_LOGGER_EXAMPLE ON)` |
| `BUILD_LOGGER_BENCHMARKS` | `ON` (standalone) | Build benchmarks (only supported for standalone builds). | `-DBUILD_LOGGER_BENCHMARKS=ON`<br>`set(BUILD_LOGGER_BENCHMARKS ON)` |
| `LOGGER_DEBUG` | `OFF` | Enables debug logging (`LOGGER_DEBUG` compile definition). Auto-enabled when `CMAKE_BUILD_TYPE=Debug`. | `-DLOGGER_DEBUG=ON`<br>`set(LOGGER_DEBUG ON)` |
| `BUILD_LOGGER_SQL` | `ON` | Enables the SQLite logging backend if `SQLite3` is found. | `-DBUILD_LOGGER_SQL=OFF`<br>`set(BUILD_LOGGER_SQL OFF)` |
| `FORCE_COLORED_OUTPUT` | `OFF` | Forces ANSI-colored compiler diagnostics (GNU/Clang only). | `-DFORCE_COLORED_OUTPUT=ON`<br>`set(FORCE_COLORED_OUTPUT ON)` |

## Using the Library in Another Project
To use the `Logger` library in your project, follow these steps:
1. **Add the Logger library as a submodule to your project**
```bash
git submodule add https://github.com/mjshakir/Logger.git external_libraries/logger
```
```bash
git submodule update --init --recursive
```
2. **Include the Logger library in your `CMakeLists.txt`**
```cmake
# Add the Logger library
add_subdirectory(external_libraries/logger)

# Add your executable
add_executable(MyProject main.cpp)

# Link the Logger library
target_link_libraries(MyProject PRIVATE Logger::logger)
```

3. **Configuring Logger Build Options**
- **Static Build**
```cmake
set(BUILD_LOGGER_SHARED_LIBS OFF)
add_subdirectory(external_libraries/logger)
```
- **Shared Build**
```cmake
set(BUILD_LOGGER_SHARED_LIBS ON)
add_subdirectory(external_libraries/logger)
```

By default, the `Logger` library is built as a shared library. You can override this by setting `BUILD_LOGGER_SHARED_LIBS` to `OFF` for a static build.

### Backend options (when using `add_subdirectory`)
```cmake
# Default backend is fmt.
set(LOGGER_FORCE_FMT ON)

# Use std::format (requires C++23 stdlib support).
# set(LOGGER_FORCE_FMT OFF)

# Optional: use std::format_string/std::format (std backend only).
# set(LOGGER_USE_STD_FORMAT_STRING ON)

add_subdirectory(external_libraries/logger)
```

## Contributing
We welcome contributions to the `Logger` project! Follow these steps to contribute:
1. **Fork the Repository:**
Go to the [Logger repository on GitHub](https://github.com/mjshakir/Logger.git).
Click the "Fork" button at the top right of the page to create a copy of the repository in your GitHub account.
2. **Clone Your Fork**
Clone your forked repository to your local machine:
```sh
git clone https://github.com/your-username/logger.git
```
```sh
cd logger
```
3. **Create a Branch**
Create a new branch for your feature or bugfix:
```sh
git checkout -b my-feature-branch
```
4. **Commit Your Changes:**
Stage and commit your changes with a meaningful commit message:
```sh
git add .
```
```sh
git commit -m "Description of your changes"
```
5. **Push Your Changes**
Push your changes to your fork on GitHub:
```sh
git push origin my-feature-branch
```
6. **Create a Pull Request**
Navigate to the original [Logger repository on GitHub](https://github.com/mjshakir/Logger.git). You should see a prompt to create a pull request for your recently pushed branch. Click "Compare & pull request" and provide a description of your changes. Then, submit the pull request.

### Guidelines
- Ensure your code adheres to the project's coding standards.
- Write clear and concise commit messages.
- Include appropriate tests for your changes.
-  Ensure your changes do not break existing functionality.
