cmake_minimum_required(VERSION 3.14)
project(MMLoggerExample VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Create example application
add_executable(mmlogger_example example.cpp)
add_executable(mmlogger_advanced_example advanced_example.cpp)
add_executable(mmlogger_optimized_glog_example optimized_glog_example.cpp)

# Complete example showing all features
add_executable(mmlogger_complete_example complete_example.cpp)
target_link_libraries(mmlogger_complete_example PRIVATE MMLogger)

# Link with MMLogger
target_link_libraries(mmlogger_example PRIVATE MMLogger)
target_link_libraries(mmlogger_advanced_example PRIVATE MMLogger)
target_link_libraries(mmlogger_optimized_glog_example PRIVATE MMLogger)
