cmake_minimum_required(VERSION 3.14)
project(MMLogger VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(MM_ENABLE_LOGGING "Enable logging functionality" ON)
option(MM_ENABLE_DEBUG "Enable debug logging" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(MM_BUILD_EXAMPLES "Build example applications" OFF)
option(MM_BUILD_TESTS "Build test applications" OFF)

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

# Library sources
set(MMLOGGER_SOURCES
    src/StdoutLogger.cpp
    src/GlogLogger.cpp
    src/LoggerFactory.cpp
    src/LoggerManager.cpp
    src/Log.cpp
)

# Library headers
set(MMLOGGER_HEADERS
    include/DisallowCopy.hpp
    include/GlogLogger.hpp
    include/ILogger.hpp
    include/ILoggerFactory.hpp
    include/Log.hpp
    include/LogBaseDef.hpp
    include/LoggerFactory.hpp
    include/LoggerManager.hpp
    include/StdoutLogger.hpp
)

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
