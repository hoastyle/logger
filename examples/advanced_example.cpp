// advanced.cpp - Advanced configuration example for MMLogger

#include <Log.hpp>
#include <LoggerManager.hpp>
#include <LogBaseDef.hpp>
#include <iostream>

int main(int argc, char* argv[]) {
    // Get the logger manager instance
    mm::LoggerManager& logManager = mm::LoggerManager::instance();
    
    // Programmatically configure the logger
    // (instead of using command line arguments)
    mm::LogConfig& config = const_cast<mm::LogConfig&>(logManager.config());
    
    // Set application identifier
    config.appId_ = "MMLoggerAdvancedExample";
    
    // Configure log levels
    config.logLevelToStderr_ = mm::detail::LogLevel_Debug;
    
    // Choose the log sink type
    config.logSinkType_ = mm::detail::LogSinkType_Stdout;
    
    // For GLog with file output, you would use:
    /*
    config.logSinkType_ = mm::detail::LogSinkType_GLog;
    config.logToFile_ = true;
    config.logFilePath_ = "/path/to/logs/";
    config.logLevelToFile_ = mm::detail::LogLevel_Info;
    */
    
    // Setup the logger
    int result = logManager.setup(argc, argv);
    if (!mm::noError(result)) {
        std::cerr << "Failed to initialize logger" << std::endl;
        return 1;
    }
    
    // Setup the logger infrastructure
    logManager.setupLogger();
    
    // Example log messages
    MM_DEBUG("This debug message will be visible because we set log level to Debug");
    MM_INFO("Starting application with advanced configuration");
    
    // Create structured log messages
    const char* username = "user123";
    int loginAttempts = 3;
    MM_INFO("User %s attempted to login %d times", username, loginAttempts);
    
    // Different log levels
    MM_WARN("This is a warning message");
    MM_ERROR("This is an error message");
    
    // Clean up
    logManager.teardown();
    
    return 0;
}
