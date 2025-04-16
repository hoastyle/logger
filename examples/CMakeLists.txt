cmake_minimum_required(VERSION 3.14)
project(MMLoggerExample VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Create example application
add_executable(mmlogger_example example.cpp)
add_executable(mmlogger_advanced_example advanced_example.cpp)

# Link with MMLogger
target_link_libraries(mmlogger_example PRIVATE MMLogger)
target_link_libraries(mmlogger_advanced_example PRIVATE MMLogger)
