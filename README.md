# MMLogger Usage Guide

MMLogger is a C++ logging library that provides flexible logging capabilities with multiple log levels and output options.

## Features

- Multiple log levels (Verbose, Debug, Info, Warn, Error, Fatal)
- Different log sinks (Stdout, Google Log)
- File logging support
- Command-line configuration
- Rate-limited logging for high-frequency events

## Basic Usage

```cpp
#include <Log.hpp>
#include <LoggerManager.hpp>

int main(int argc, char* argv[]) {
    // Initialize the logger
    mm::LoggerManager& logManager = mm::LoggerManager::instance();
    logManager.setup(argc, argv);
    logManager.setupLogger();
    
    // Log messages
    MM_INFO("Hello, MMLogger!");
    MM_DEBUG("This is a debug message");
    MM_WARN("This is a warning");
    MM_ERROR("This is an error");
    
    // Clean up
    logManager.teardown();
    return 0;
}
```

## Log Levels

The library supports the following log levels in ascending order of severity:

- `LogLevel_Verbose`: Detailed information for debugging
- `LogLevel_Debug`: Debugging information
- `LogLevel_Info`: Normal runtime information
- `LogLevel_Warn`: Warning conditions
- `LogLevel_Error`: Error conditions
- `LogLevel_Fatal`: Critical errors causing termination

## Command-Line Configuration

The logger can be configured via command-line arguments:

```
./your_app --toTerm=debug --sinktype=Stdout
./your_app --toTerm=info --sinktype=GLog --file=true --filepath=/path/to/logs/ --toFile=error
```

Available options:
- `--toTerm=<level>`: Set console log level (verbose, debug, info, warn, error, fatal)
- `--toFile=<level>`: Set file log level (verbose, debug, info, warn, error, fatal)
- `--sinktype=<type>`: Set log sink type (Stdout, GLog)
- `--file=<bool>`: Enable/disable file logging (true, false)
- `--filepath=<path>`: Set log file path
- `--appid=<name>`: Set application identifier
- `--debugSwitch=<bool>`: Enable/disable debug logging (true, false)

## CMake Integration

```cmake
# Find MMLogger
find_package(MMLogger REQUIRED)

# Link your application with MMLogger
target_link_libraries(your_app PRIVATE MM::MMLogger)
```

## Advanced Features

### Rate-Limited Logging

For high-frequency events, use rate-limited logging to avoid flooding logs:

```cpp
mm::detail::RateLimitedLog rateLimitedLogger(std::chrono::milliseconds(1000));

// This will only log once per second
for (int i = 0; i < 100; i++) {
    rateLimitedLogger.Log(mm::detail::LogLevel_Info, "Processing item %d", i);
    // Do frequent work...
}
```

### Programmatic Configuration

Instead of using command-line arguments, you can programmatically configure the logger:

```cpp
mm::LoggerManager& logManager = mm::LoggerManager::instance();
mm::LogConfig& config = const_cast<mm::LogConfig&>(logManager.config());

config.appId_ = "MyApplication";
config.logLevelToStderr_ = mm::detail::LogLevel_Debug;
config.logSinkType_ = mm::detail::LogSinkType_GLog;
config.logToFile_ = true;
config.logFilePath_ = "/var/log/myapp/";
config.logLevelToFile_ = mm::detail::LogLevel_Info;

logManager.setup(argc, argv);
logManager.setupLogger();
```

## Build and Installation

CMake is used for building and installing the library:

```bash
mkdir build && cd build
cmake ..
make
sudo make install
```
