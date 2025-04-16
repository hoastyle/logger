# CMake Integration
```cmake
# Find MMLogger package
find_package(MMLogger REQUIRED)

# Link your application with MMLogger
target_link_libraries(your_application PRIVATE MMLogger)
```

# Header Integration
In your C++ files, include the necessary headers:
```cpp
#include <Log.hpp>               // For logging macros
#include <LoggerManager.hpp>     // For logger configuration
```

# Basic Usage
Here's a minimal example showing how to initialize and use MMLogger:
```cpp
#include <Log.hpp>
#include <LoggerManager.hpp>

int main(int argc, char* argv[]) {
    // Initialize the logger manager (singleton)
    mm::LoggerManager& logManager = mm::LoggerManager::instance();
    
    // Setup logger with command line arguments
    logManager.setup(argc, argv);
    
    // Initialize logger infrastructure
    logManager.setupLogger();
    
    // Use logging macros
    MM_INFO("Application starting with PID: %d", logManager.pid());
    MM_DEBUG("Debug information");
    MM_WARN("Warning message");
    MM_ERROR("Error message");
    
    // Clean up on shutdown
    logManager.teardown();
    
    return 0;
}
```
