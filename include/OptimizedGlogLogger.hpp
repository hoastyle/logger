/**
 * SHANGHAI MASTER MATRIX CONFIDENTIAL
 * Copyright 2018-2023 Shanghai Master Matrix Corporation All Rights Reserved.
 */

#ifndef INCLUDE_COMMON_LOG_OPTIMIZEDGLOGLOGGER_HPP_
#define INCLUDE_COMMON_LOG_OPTIMIZEDGLOGLOGGER_HPP_

#include <glog/logging.h>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>
#include <chrono>

#include "ILogger.hpp"
#include "DisallowCopy.hpp"

namespace mm {

// Forward declarations
class LogMessagePool;

/**
 * @brief A high-performance async logger implementation using Google's glog
 *
 * This logger implements an optimized logging strategy designed for
 * high-throughput multi-threaded environments. Features include:
 * - Asynchronous logging with non-blocking write operations
 * - Memory pooling to reduce allocations
 * - Batch processing to reduce I/O operations
 * - Smart message dropping during overload
 * - Configurable worker threads for log processing
 */
class OptimizedGlogLogger final : public ILogger {
 public:
  /**
   * @brief Structure representing a log message in the queue
   */
  struct LogMessage {
    detail::LogLevel level;
    const char* msg;  // Points to memory in the pool
    std::size_t len;
    LogMessage* next;  // For memory pool linked list
    size_t bufferIndex;
  };

  /**
   * @brief Constructor for OptimizedGlogLogger
   *
   * @param appId Application identifier
   * @param logLevelToStderr Log level for stderr output
   * @param logLevelToFile Log level for file output
   * @param logToFile Whether to log to file
   * @param logFilePath Path for log files
   * @param logDebugSwitch Whether to enable debug logs
   * @param batchSize Number of messages to process in a batch (default: 100)
   * @param queueCapacity Maximum queue size before dropping messages (default:
   * 10000)
   * @param numWorkers Number of worker threads (default: 2)
   * @param poolSize Size of the memory pool (default: 10000)
   */
  OptimizedGlogLogger(const std::string& appId,
      const detail::LogLevel logLevelToStderr,
      const detail::LogLevel logLevelToFile, const LogToFile logToFile,
      const LogFilePath logFilePath, const LogDebugSwitch logDebugSwitch,
      const bool logToConsole = false, size_t batchSize = 100,
      size_t queueCapacity = 10000, size_t numWorkers = 2,
      size_t poolSize = 10000) noexcept;

  virtual ~OptimizedGlogLogger() override;

  virtual int setup() override;
  virtual int teardown() override;

  virtual void logVerbose(const char* msg, const std::size_t len) override;
  virtual void logDebug(const char* msg, const std::size_t len) override;
  virtual void logInfo(const char* msg, const std::size_t len) override;
  virtual void logWarn(const char* msg, const std::size_t len) override;
  virtual void logError(const char* msg, const std::size_t len) override;
  virtual void logFatal(const char* msg, const std::size_t len) override;

 private:
  /**
   * @brief Converts internal log level to glog level
   */
  int convertLogLevel(detail::LogLevel level) noexcept;

  /**
   * @brief Worker thread function that processes log messages
   */
  void workerThread();

  /**
   * @brief Enqueues a log message for async processing
   *
   * @param level Log level
   * @param msg Log message
   * @param len Length of the message
   * @return true if message was enqueued, false if dropped
   */
  bool enqueueLogMessage(
      detail::LogLevel level, const char* msg, std::size_t len);

  /**
   * @brief Process a batch of log messages
   */
  void processLogBatch();

  /**
   * @brief Determines if a message should be dropped based on priority and
   * queue state
   */
  bool shouldDropMessage(detail::LogLevel level) const;

  /**
   * @brief Memory pool for log messages to avoid allocations
   */
  class LogMessagePool {
   public:
    LogMessagePool(size_t poolSize, size_t msgBufferSize);
    ~LogMessagePool();

    // Get a log message with buffer for the message content
    LogMessage* acquireLogMessage(const char* msg, size_t len);

    // Return a log message to the pool
    void releaseLogMessage(LogMessage* logMsg);

   private:
    std::mutex poolMutex_;
    LogMessage* freeList_;
    std::vector<char*> buffers_;
    std::vector<LogMessage*> messages_;
    size_t msgBufferSize_;

    MM_DISALLOW_COPY_AND_MOVE(LogMessagePool)
  };

  // Logger configuration
  std::string appId_;
  detail::LogLevel logLevelToStderr_;
  detail::LogLevel logLevelToFile_;
  LogToFile logToFile_;
  LogFilePath logFilePath_;
  LogDebugSwitch logDebugSwitch_;
  bool logToConsole_;

  // Performance configuration
  const size_t batchSize_;
  const size_t queueCapacity_;
  const size_t numWorkers_;
  const size_t msgBufferSize_ = 2048;  // Max message size

  // Worker threads and synchronization
  std::vector<std::thread> workers_;
  std::queue<LogMessage*> messageQueue_;
  std::mutex queueMutex_;
  std::condition_variable queueCV_;
  std::atomic<bool> shutdown_;

  // Memory management
  std::unique_ptr<LogMessagePool> messagePool_;

  // Performance metrics
  std::atomic<uint64_t> enqueuedCount_;
  std::atomic<uint64_t> processedCount_;
  std::atomic<uint64_t> droppedCount_;
  std::atomic<uint64_t> overflowCount_;

  // Rate limiting for log types
  struct RateLimitEntry {
    std::chrono::steady_clock::time_point lastLogTime;
    std::chrono::milliseconds interval;
  };
  std::unordered_map<std::string, RateLimitEntry> rateLimits_;
  std::mutex rateLimitMutex_;

  MM_DISALLOW_COPY_AND_MOVE(OptimizedGlogLogger)
};

}  // namespace mm

#endif  // INCLUDE_COMMON_LOG_OPTIMIZEDGLOGLOGGER_HPP_
