# MMLogger Build Script Guide

This document explains how to use the `build.sh` script for compiling the MMLogger library.

## Overview

The `build.sh` script simplifies the process of configuring, building, and testing the MMLogger library. It provides a convenient command-line interface for customizing various build options without having to remember complex CMake commands.

## Basic Usage

To build the library with default settings:

```bash
./build.sh
```

This will create a `build` directory, configure the project with CMake, and build the library.

## Available Options

The script supports the following command-line options:

| Option | Description | Default |
|--------|-------------|---------|
| `--debug` | Build in Debug mode (with debug symbols) | Release |
| `--prefix=<path>` | Set installation directory | /usr/local |
| `--static` | Build static libraries instead of shared | Shared libraries |
| `--examples` | Build example applications | OFF |
| `--tests` | Build test applications | OFF |
| `--enable-debug-logs` | Enable debug logging | OFF |
| `--help` | Display usage information | - |

## Examples

### Building a debug version

```bash
./build.sh --debug
```

### Building with examples

```bash
./build.sh --examples
```

### Building a static library with custom install location

```bash
./build.sh --static --prefix=/opt/mmlogger
```

### Building with all options

```bash
./build.sh --debug --examples --tests --enable-debug-logs
```

## Output Information

After a successful build, the script displays information about the generated files:

1. **Library Files**: Paths to the compiled library files (`.so`, `.a`, etc.)
2. **Header Files**: Locations of the header files in the source tree
3. **Example Files** (if built): Locations of example executables and source files

## Installation

To install the library after building:

```bash
cd build && sudo cmake --build . --target install
```

## Troubleshooting

### Script not executing

If you encounter permission issues, make the script executable:

```bash
chmod +x build.sh
```

### CMake errors

If you see CMake errors, ensure you have all required dependencies installed:

```bash
sudo apt install cmake build-essential libgoogle-glog-dev
```

### Build errors

If compilation fails, try building in debug mode to get more detailed error information:

```bash
./build.sh --debug
```

## Advanced Usage

### Using with CI/CD pipelines

The script can be easily integrated into CI/CD pipelines:

```bash
./build.sh --static --tests && cd build && ctest
```

### Building with custom compiler

Use environment variables to specify custom compilers:

```bash
CC=clang CXX=clang++ ./build.sh
```
