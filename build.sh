#!/bin/bash
# Script to build the MMLogger library

# Get the script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

# Default values
BUILD_TYPE="Release"
INSTALL_PREFIX="/usr/local"
BUILD_SHARED=ON
BUILD_EXAMPLES=OFF
BUILD_TESTS=OFF
ENABLE_DEBUG=OFF
INSTALL_LIB=OFF

# Process command line arguments
while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --prefix=*)
            INSTALL_PREFIX="${key#*=}"
            shift
            ;;
        --static)
            BUILD_SHARED=OFF
            shift
            ;;
        --examples)
            BUILD_EXAMPLES=ON
            shift
            ;;
        --tests)
            BUILD_TESTS=ON
            shift
            ;;
        --enable-debug-logs)
            ENABLE_DEBUG=ON
            shift
            ;;
        --install)
            INSTALL_LIB=ON
            shift
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --debug                 Build in Debug mode instead of Release"
            echo "  --prefix=<path>         Set the installation prefix (default: /usr/local)"
            echo "  --static                Build static libraries instead of shared"
            echo "  --examples              Build example applications"
            echo "  --tests                 Build test applications"
            echo "  --enable-debug-logs     Enable debug logging"
            echo "  --install               Install the library after building"
            echo "  --help                  Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $key"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Create and enter build directory
mkdir -p build
cd build || exit 1

# Configure project
cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
    -DBUILD_SHARED_LIBS="$BUILD_SHARED" \
    -DMM_BUILD_EXAMPLES="$BUILD_EXAMPLES" \
    -DMM_BUILD_TESTS="$BUILD_TESTS" \
    -DMM_ENABLE_DEBUG="$ENABLE_DEBUG"

# Build project
cmake --build . -- -j "$(nproc)"

# Check if build was successful
if [ $? -ne 0 ]; then
    echo "Build failed."
    exit 1
fi

echo "Build completed successfully."

# Display the generated files
echo
echo "Generated files:"
echo "----------------"
echo "Library files:"
find "$(pwd)" -name "libMMLogger.*" | while read -r file; do
    echo "  - $file"
done

echo
echo "Header files (in source):"
find "${SCRIPT_DIR:-..}/include" -name "*.hpp" | while read -r file; do
    echo "  - $file"
done

# Display examples if they were built
if [ "$BUILD_EXAMPLES" = "ON" ]; then
    echo
    echo "Example executables:"
    find "$(pwd)" -path "*/examples/*" -type f -executable | while read -r file; do
        echo "  - $file"
    done

    # Also find example source files
    echo
    echo "Example source files:"
    find "${SCRIPT_DIR:-..}/examples" -name "*.cpp" | while read -r file; do
        echo "  - $file"
    done
fi

# Install the library if requested
if [ "$INSTALL_LIB" = "ON" ]; then
    echo
    echo "Installing MMLogger to $INSTALL_PREFIX..."

    # Check if we need sudo (only if installing to system directories)
    if [[ "$INSTALL_PREFIX" == "/usr"* || "$INSTALL_PREFIX" == "/opt"* ]]; then
        # Check if user has write access to the installation directory
        if [ ! -w "$INSTALL_PREFIX" ]; then
            echo "Installing to system directory. This may require sudo privileges."
            sudo cmake --build . --target install
        else
            cmake --build . --target install
        fi
    else
        # For user-defined directories, don't use sudo
        cmake --build . --target install
    fi

    if [ $? -eq 0 ]; then
        echo "Installation completed successfully."
        echo
        echo "Installed files:"
        echo "----------------"

        # List installed library files
        find "$INSTALL_PREFIX/lib" -name "libMMLogger.*" 2>/dev/null | while read -r file; do
            echo "  - $file"
        done

        # List installed header files (specifically for MMLogger in the /mm directory)
        find "$INSTALL_PREFIX/include/mm" -name "*.hpp" 2>/dev/null | while read -r file; do
            echo "  - $file"
        done

        # List CMake config files
        find "$INSTALL_PREFIX/lib/cmake/MMLogger" -name "*.cmake" 2>/dev/null | while read -r file; do
            echo "  - $file"
        done
    else
        echo "Installation failed."
        exit 1
    fi
else
    echo
    echo "To install the library, run:"
    echo "  $0 --install"
    echo "Or manually:"
    echo "  cd build && sudo cmake --build . --target install"
fi
