# Logger

## Summary

The Logger library is a lightweight, fast, and efficient logging utility for C++ projects. It provides various log levels (debug, error, warning, normal), supports color-coded output, and allows for conditional compilation to use either the standard `std::format` or the `fmt` library for formatting.

## Features

- Different log levels: Debug, Error, Warning, Normal
- Color-coded console output for errors and warnings
- Conditional compilation to use `std::format` or `fmt` library
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
You can use `vcpkg` to install `fmt` on Windows.
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
cd logger
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
add_subdirectory(external/logger)

# Include directories
include_directories(${LOGGER_INCLUDE_DIR})

# Add your executable
add_executable(MyProject main.cpp)

# Link the Logger library
target_link_libraries(MyProject PRIVATE ${LOGGER_LIBRARIES})
```

3. **Configuring Logger Build Options**
- **Static Build**
```cmake
set(BUILD_LOGGER_SHARED_LIBS OFF)
add_subdirectory(external/logger)
```
- **Shared Build**
```cmake
set(BUILD_LOGGER_SHARED_LIBS ON)
add_subdirectory(external/logger)
```

By default, the `Logger` library is built as a shared library. You can override this by setting `BUILD_LOGGER_SHARED_LIBS` to `OFF` for a static build.

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