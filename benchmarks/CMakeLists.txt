cmake_minimum_required(VERSION 3.14)
project(MMLoggerExample VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Create example application AND Link with MMLogger
add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark PRIVATE MMLogger)
