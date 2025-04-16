// Example of how to use the optimized logger

#include <Log.hpp>
#include <LoggerManager.hpp>
#include <chrono>
#include <thread>
#include <vector>

// Function to generate logs at high rate
void logGenerationThread(int threadId, int iterations) {
  for (int i = 0; i < iterations; ++i) {
    // Vary the log levels
    if (i % 100 == 0) {
      MM_ERROR("Thread %d: Critical operation failed with error code %d",
          threadId, i);
    } else if (i % 20 == 0) {
      MM_WARN("Thread %d: Warning condition detected, value=%d", threadId, i);
    } else {
      MM_INFO("Thread %d: Processing item %d", threadId, i);
    }

    // Optional: add some processing time to simulate real work
    if (i % 1000 == 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
}

int main(int argc, char* argv[]) {
  // Setup with command line arguments or programmatically

  // Method 1: Use command line arguments
  // ./your_app --sinktype=OptimizedGLog --toTerm=info --batchSize=200
  // --numWorkers=4

  // Method 2: Configure programmatically
  if (argc == 1) {
    // Create dummy argv for default settings
    static char prog[]    = "myapp";
    static char sink[]    = "--sinktype=OptimizedGLog";
    static char level[]   = "--toTerm=info";
    static char batch[]   = "--batchSize=200";
    static char workers[] = "--numWorkers=4";
    static char queue[]   = "--queueCapacity=20000";

    static char* myargv[] = {prog, sink, level, batch, workers, queue};
    argc                  = 6;
    argv                  = myargv;
  }

  // Initialize the logger
  mm::LoggerManager& logManager = mm::LoggerManager::instance();
  logManager.setup(argc, argv);
  logManager.setupLogger();

  // Log some initial messages
  MM_INFO("Application starting with OptimizedGlogLogger");
  MM_INFO("PID: %d", logManager.pid());

  // Create multiple threads that generate high volumes of logs
  std::vector<std::thread> threads;
  const int numThreads = 10;
  const int logsPerThread =
      100000;  // Each thread will generate this many log entries

  MM_INFO("Starting %d threads to generate %d log entries each", numThreads,
      logsPerThread);

  auto startTime = std::chrono::steady_clock::now();

  // Start threads
  for (int i = 0; i < numThreads; ++i) {
    threads.emplace_back(logGenerationThread, i, logsPerThread);
  }

  // Wait for all threads to complete
  for (auto& thread : threads) {
    thread.join();
  }

  auto endTime  = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      endTime - startTime);

  MM_INFO("All threads completed");
  MM_INFO("Generated %d log entries in %ld ms", numThreads * logsPerThread,
      duration.count());
  MM_INFO("Rate: %f logs/second",
      (numThreads * logsPerThread * 1000.0) / duration.count());

  // Clean up
  logManager.teardown();

  return 0;
}
