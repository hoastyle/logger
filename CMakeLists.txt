cmake_minimum_required(VERSION 3.14)
project(MMLogger VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Options
option(MM_ENABLE_LOGGING "Enable logging functionality" ON)
option(MM_ENABLE_DEBUG "Enable debug logging" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(MM_BUILD_EXAMPLES "Build example applications" OFF)
option(MM_BUILD_TESTS "Build test applications" OFF)
option(MM_BUILD_BENCHMARKS "Build benchmark applications" OFF) # 新增的选项

# Define compile flags
if(MM_ENABLE_LOGGING)
    add_compile_definitions(MM_ENABLE_LOGGING)
endif()

if(MM_ENABLE_DEBUG)
    add_compile_definitions(MM_ENABLE_DEBUG)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Find required dependencies
find_package(Threads REQUIRED)
find_package(glog REQUIRED)

file(GLOB_RECURSE MMLOGGER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# Library headers
file(GLOB_RECURSE MMLOGGER_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")

# Create library
add_library(MMLogger ${MMLOGGER_SOURCES})

# Set library properties
set_target_properties(MMLogger PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${MMLOGGER_HEADERS}"
)

# Link dependencies
target_link_libraries(MMLogger
    PUBLIC
        Threads::Threads
        glog::glog
)

# Include directories for build and install interface
target_include_directories(MMLogger
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

# Installation rules
include(GNUInstallDirs)

install(TARGETS MMLogger
    EXPORT MMLoggerTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mm
)

# Generate and install export file
install(EXPORT MMLoggerTargets
    FILE MMLoggerTargets.cmake
    NAMESPACE MM::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MMLogger
)

# Create config file
include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/MMLoggerConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/MMLoggerConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MMLogger
)

# Generate version file
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/MMLoggerConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install config files
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/MMLoggerConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/MMLoggerConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MMLogger
)

# Build examples if requested
if(MM_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Build tests if requested
if(MM_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Build benchmarks if requested
if(MM_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()
