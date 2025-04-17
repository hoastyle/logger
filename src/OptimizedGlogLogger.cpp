/**
 * SHANGHAI MASTER MATRIX CONFIDENTIAL
 * Copyright 2018-2023 Shanghai Master Matrix Corporation All Rights Reserved.
 */

#include "OptimizedGlogLogger.hpp"
#include "Log.hpp"
#include "LoggerStatus.hpp"

#include <cstring>
#include <algorithm>
#include <unistd.h>
#include <sys/syscall.h>

namespace mm {

OptimizedGlogLogger::LogMessagePool::LogMessagePool(
    size_t poolSize, size_t msgBufferSize)
    : freeList_(nullptr), msgBufferSize_(msgBufferSize) {
  messages_.reserve(poolSize);
  buffers_.reserve(poolSize);

  for (size_t i = 0; i < poolSize; ++i) {
    char* buffer = new char[msgBufferSize];
    buffers_.push_back(buffer);

    LogMessage* msg  = new LogMessage{};
    msg->bufferIndex = i;
    msg->next        = freeList_;
    freeList_        = msg;
    messages_.push_back(msg);
  }
}

OptimizedGlogLogger::LogMessagePool::~LogMessagePool() {
  for (auto msg : messages_) {
    delete msg;
  }
  messages_.clear();

  for (auto buffer : buffers_) {
    delete[] buffer;
  }
  buffers_.clear();
}

// 修改获取消息的方法
OptimizedGlogLogger::LogMessage*
OptimizedGlogLogger::LogMessagePool::acquireLogMessage(
    const char* msg, size_t len) {
  std::lock_guard<std::mutex> lock(poolMutex_);

  if (freeList_ == nullptr) {
    return nullptr;
  }

  LogMessage* logMsg = freeList_;
  freeList_          = freeList_->next;

  size_t bufferIndex = logMsg->bufferIndex;

  if (bufferIndex >= buffers_.size()) {
    logMsg->next = freeList_;
    freeList_    = logMsg;
    return nullptr;
  }

  char* buffer = buffers_[bufferIndex];

  size_t copyLen = std::min(len, msgBufferSize_ - 1);
  std::memcpy(buffer, msg, copyLen);
  buffer[copyLen] = '\0';

  logMsg->msg  = buffer;
  logMsg->len  = copyLen;
  logMsg->next = nullptr;

  return logMsg;
}

void OptimizedGlogLogger::LogMessagePool::releaseLogMessage(
    LogMessage* logMsg) {
  if (!logMsg) return;

  std::lock_guard<std::mutex> lock(poolMutex_);

  logMsg->next = freeList_;
  freeList_    = logMsg;
}

// Implementation of OptimizedGlogLogger
OptimizedGlogLogger::OptimizedGlogLogger(const std::string& appId,
    const detail::LogLevel logLevelToStderr,
    const detail::LogLevel logLevelToFile, const LogToFile logToFile,
    const LogFilePath logFilePath, const LogDebugSwitch logDebugSwitch,
    const bool logToConsole, size_t batchSize, size_t queueCapacity,
    size_t numWorkers, size_t poolSize) noexcept
    : appId_(appId),
      logLevelToStderr_(logLevelToStderr),
      logLevelToFile_(logLevelToFile),
      logToFile_(logToFile),
      logFilePath_(logFilePath),
      logDebugSwitch_(logDebugSwitch),
      logToConsole_(logToConsole),
      batchSize_(batchSize),
      queueCapacity_(queueCapacity),
      numWorkers_(numWorkers),
      shutdown_(false),
      enqueuedCount_(0),
      processedCount_(0),
      droppedCount_(0),
      overflowCount_(0) {
  // Initialize path for logs
  if (logToFile_) {
    if (logFilePath_.empty()) {
      char curAbsPath[256];
      if (nullptr != getcwd(curAbsPath, 256)) {
        logFilePath_ = curAbsPath;
        logFilePath_ += "/glogs/";
      }
    }
  }

  // Create message pool
  messagePool_ = std::make_unique<LogMessagePool>(poolSize, msgBufferSize_);
}

OptimizedGlogLogger::~OptimizedGlogLogger() { teardown(); }

int OptimizedGlogLogger::setup() {
  int ec = MM_STATUS_OK;

  // Initialize Google logging
  google::InitGoogleLogging(appId_.c_str());

  // Configure glog for optimal performance
  google::EnableLogCleaner(GLOG_OVERDUE_DAY);

  if (logToConsole_) {
    // default to file
    FLAGS_logtostderr = false;
    // output to console & file
    FLAGS_alsologtostderr = true;
    // control level to console
    FLAGS_stderrthreshold = convertLogLevel(logLevelToStderr_);
    // enable color output
    FLAGS_colorlogtostderr = true;
  } else {
    // disable console output
    FLAGS_logtostderr = false;
    // disable output to console & file at the same time
    FLAGS_alsologtostderr = false;
    FLAGS_stderrthreshold = google::GLOG_FATAL;
  }

  // Performance optimizations for glog
  FLAGS_logbufsecs                = 0;     // Flush log file after each write
  FLAGS_max_log_size              = 1024;  // 1GB per log file
  FLAGS_stop_logging_if_full_disk = true;

  // Set log file destination if needed
  if (logToFile_) {
    if (createAbsDirectory(logFilePath_)) {
      for (auto logLevel = logLevelToFile_;
           logLevel <= detail::LogLevel::LogLevel_Fatal;
           logLevel = detail::LogLevel(logLevel << 1)) {
        google::SetLogDestination(
            convertLogLevel(logLevel), logFilePath_.c_str());
      }
    } else {
      ec = MM_STATUS_ENOENT;
      std::fprintf(
          stderr, "Failed to create log directory: %s\n", logFilePath_.c_str());
      return ec;
    }
  }

  // Start worker threads for async processing
  shutdown_ = false;
  for (size_t i = 0; i < numWorkers_; ++i) {
    workers_.emplace_back(&OptimizedGlogLogger::workerThread, this);
  }

  return ec;
}

int OptimizedGlogLogger::teardown() {
  // Signal shutdown to worker threads
  {
    std::lock_guard<std::mutex> lock(queueMutex_);
    shutdown_ = true;
  }
  queueCV_.notify_all();

  // Wait for worker threads to complete
  for (auto& worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }
  workers_.clear();

  // Process any remaining messages in the queue
  processLogBatch();

  // Clean up message queue
  {
    std::lock_guard<std::mutex> lock(queueMutex_);
    while (!messageQueue_.empty()) {
      LogMessage* msg = messageQueue_.front();
      messageQueue_.pop();
      messagePool_->releaseLogMessage(msg);
    }
  }

  // Shutdown glog
  google::ShutdownGoogleLogging();

  // Log performance metrics
  std::fprintf(stderr,
      "OptimizedGlogLogger stats - Enqueued: %lu, Processed: %lu, Dropped: "
      "%lu, Overflow: %lu\n",
      static_cast<unsigned long>(enqueuedCount_.load()),
      static_cast<unsigned long>(processedCount_.load()),
      static_cast<unsigned long>(droppedCount_.load()),
      static_cast<unsigned long>(overflowCount_.load()));
  return MM_STATUS_OK;
}

void OptimizedGlogLogger::workerThread() {
  while (true) {
    // Wait for work or shutdown signal
    {
      std::unique_lock<std::mutex> lock(queueMutex_);
      queueCV_.wait(lock, [this] {
        return shutdown_ || messageQueue_.size() >= batchSize_ ||
               (!messageQueue_.empty() &&
                   messageQueue_.size() >= queueCapacity_ / 2);
      });

      // Exit if shutdown and no more messages
      if (shutdown_ && messageQueue_.empty()) {
        break;
      }
    }

    // Process a batch of messages
    processLogBatch();
  }
}

void OptimizedGlogLogger::processLogBatch() {
  std::vector<LogMessage*> batch;
  batch.reserve(batchSize_);

  // Get a batch of messages from the queue with minimum lock time
  {
    std::lock_guard<std::mutex> lock(queueMutex_);
    size_t count = std::min(messageQueue_.size(), batchSize_);
    for (size_t i = 0; i < count; ++i) {
      batch.push_back(messageQueue_.front());
      messageQueue_.pop();
    }
  }

  // Process each message in the batch
  for (LogMessage* msg : batch) {
    switch (msg->level) {
      case detail::LogLevel_Debug:
        if (logDebugSwitch_) {
          VLOG(1) << msg->msg;  // Use VLOG for debug messages for better
                                // performance
        }
        break;
      case detail::LogLevel_Info: LOG(INFO) << msg->msg; break;
      case detail::LogLevel_Warn: LOG(WARNING) << msg->msg; break;
      case detail::LogLevel_Error: LOG(ERROR) << msg->msg; break;
      case detail::LogLevel_Fatal: LOG(FATAL) << msg->msg; break;
      default:
        // Ignore other levels
        break;
    }

    // Return the message to the pool
    messagePool_->releaseLogMessage(msg);
    processedCount_++;
  }
}

bool OptimizedGlogLogger::shouldDropMessage(detail::LogLevel level) const {
  // Always process fatal logs
  if (level == detail::LogLevel_Fatal) {
    return false;
  }

  // Check queue capacity - apply back pressure when queue gets full
  size_t currentQueueSize = messageQueue_.size();

  if (currentQueueSize >= queueCapacity_) {
    // Queue is full, drop based on priority
    if (level <= detail::LogLevel_Debug) {
      return true;  // Always drop debug and verbose when overloaded
    }

    if (currentQueueSize >= queueCapacity_ * 1.2) {
      // Severe overload: drop everything except Error and Fatal
      return level < detail::LogLevel_Error;
    }

    // Moderate overload: drop everything except Warn, Error and Fatal
    return level < detail::LogLevel_Warn;
  }

  return false;
}

bool OptimizedGlogLogger::enqueueLogMessage(
    detail::LogLevel level, const char* msg, std::size_t len) {
  // Check if message should be dropped based on level and queue state
  if (shouldDropMessage(level)) {
    droppedCount_++;
    return false;
  }

  // Get a log message from the pool
  LogMessage* logMsg = messagePool_->acquireLogMessage(msg, len);
  if (!logMsg) {
    overflowCount_++;
    return false;
  }

  // Set the log level
  logMsg->level = level;

  // Add to queue with minimal lock time
  {
    std::lock_guard<std::mutex> lock(queueMutex_);
    messageQueue_.push(logMsg);
  }

  enqueuedCount_++;

  // Notify a worker thread
  queueCV_.notify_one();

  return true;
}

int OptimizedGlogLogger::convertLogLevel(detail::LogLevel level) noexcept {
  switch (level) {
    case detail::LogLevel_Debug: return google::GLOG_INFO;
    case detail::LogLevel_Info: return google::GLOG_INFO;
    case detail::LogLevel_Warn: return google::GLOG_WARNING;
    case detail::LogLevel_Error: return google::GLOG_ERROR;
    case detail::LogLevel_Fatal: return google::GLOG_FATAL;
    default: return google::GLOG_INFO;
  }
}

void OptimizedGlogLogger::logVerbose(const char* msg, const std::size_t len) {
  // Verbose is not directly supported by glog, map to VLOG(2)
  if (enqueueLogMessage(detail::LogLevel_Debug, msg, len)) {
    // Successfully enqueued
  }
}

void OptimizedGlogLogger::logDebug(const char* msg, const std::size_t len) {
  if (logDebugSwitch_) {
    if (enqueueLogMessage(detail::LogLevel_Debug, msg, len)) {
      // Successfully enqueued
    }
  }
}

void OptimizedGlogLogger::logInfo(const char* msg, const std::size_t len) {
  if (enqueueLogMessage(detail::LogLevel_Info, msg, len)) {
    // Successfully enqueued
  }
}

void OptimizedGlogLogger::logWarn(const char* msg, const std::size_t len) {
  if (enqueueLogMessage(detail::LogLevel_Warn, msg, len)) {
    // Successfully enqueued
  }
}

void OptimizedGlogLogger::logError(const char* msg, const std::size_t len) {
  if (enqueueLogMessage(detail::LogLevel_Error, msg, len)) {
    // Successfully enqueued
  }
}

void OptimizedGlogLogger::logFatal(const char* msg, const std::size_t len) {
  // Fatal logs bypass the queue and are logged immediately
  LOG(FATAL) << msg;
}

}  // namespace mm
