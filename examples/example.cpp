// main.cpp - Basic example for using MMLogger

#include <Log.hpp>
#include <LoggerManager.hpp>

#include <iostream>
#include <thread>
#include <chrono>

void performTask() {
  // Log from a different function
  MM_INFO("Performing a computation task");

  // Simulate some work
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  double result = 42.0;
  MM_INFO("Task completed with result: %.2f", result);
}

int main(int argc, char* argv[]) {
  // Initialize the logger manager (singleton)
  mm::LoggerManager& logManager = mm::LoggerManager::instance();

  // Setup the logger with command line arguments
  // You can pass arguments like:
  //   --toTerm=debug     - Set console log level
  //   --sinktype=Stdout  - Use stdout logger (or GLog)
  int result = logManager.setup(argc, argv);
  if (!mm::noError(result)) {
    std::cerr << "Failed to initialize logger" << std::endl;
    return 1;
  }

  // Setup the logger infrastructure
  logManager.setupLogger();

  // Start the logger
  logManager.Start();

  // Log messages at different levels
  MM_INFO("Application starting with PID: %d", logManager.pid());
  MM_DEBUG("Debug information: Log level configured successfully");

  // Check a condition and log a warning if needed
  bool systemCheck = false;
  if (!systemCheck) {
    MM_WARN("System check failed, proceeding with caution");
  }

  // Log error for a simulated failure
  bool connectionFailed = true;
  if (connectionFailed) {
    MM_ERROR("Failed to connect to network service: timeout");
  }

  // Perform and log a task
  performTask();

  // Use rate-limited logging for high-frequency events
  mm::detail::RateLimitedLog rateLimitedLogger(std::chrono::milliseconds(1000));
  for (int i = 0; i < 5; i++) {
    // This will only log once per second regardless of how many times it's
    // called
    rateLimitedLogger.Log(
        mm::detail::LogLevel_Info, "Processing data batch %d", i);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  // Log application shutdown
  MM_INFO("Application shutting down");

  // Clean up
  logManager.teardown();

  return 0;
}
