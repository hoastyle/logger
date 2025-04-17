/**
 * SHANGHAI MASTER MATRIX CONFIDENTIAL
 * Copyright 2018-2023 Shanghai Master Matrix Corporation All Rights Reserved.

 * The source code, information and material ("Material") contained herein is
 * owned by Shanghai Master Matrix Corporation or its suppliers and licensors,
 * and title to such Material remains with Shanghai Master Matrix Corporation,
 * its suppliers or licensors. This Material contains proprietary information
 * from Shanghai Master Matrix Corporation or its suppliers and its licensors.
 * The Material is protected by worldwide copyright laws and treaty provision.
 * No part of the Material could be used, copied, published, modified, posted,
 * uploaded, reproduced, transmitted, distributed or disclosed anyway without
 * Shanghai Master Matrix's prior express written permission.No license under
 * any patent, copyright or other intellectual property right in the Material
 * is granted to or conferred upon you, either expressly, by any implications,
 * inducement, estoppel or otherwise. Any license under intellectual property
 * rights must be authorized by Shanghai Master Matrix Corporation in writing.
 *
 * Unless otherwise agreed by Shanghai Master Matrix in writing, you must not
 * remove or alter this notice or any other notices embedded in this Material
 * by Shanghai Master Matrix Corporation or its suppliers or licensors anyway.
 */

#include "GlogLogger.hpp"

#include "Log.hpp"
#include "LoggerStatus.hpp"

namespace mm {

GlogLogger::GlogLogger(const std::string& appId,
    const detail::LogLevel logLevelToStderr,
    const detail::LogLevel logLevelToFile, const LogToFile logToFile,
    const LogFilePath logFilePath, const LogDebugSwitch logDebugSwitch,
    const bool logToConsole) noexcept
    : appId_(appId),
      logLevelToStderr_(logLevelToStderr),
      logLevelToFile_(logLevelToFile),
      logToFile_(logToFile),
      logFilePath_(logFilePath),
      logDebugSwitch_(logDebugSwitch),
      logToConsole_(logToConsole) {
  if (logToFile_) {
    if (logFilePath_.empty()) {
      /* get current absolute path */
      char curAbsPath[DefaultMaxPathSize];
      if (nullptr != getcwd(curAbsPath, DefaultMaxPathSize)) {
        logFilePath_ = curAbsPath;
        logFilePath_ += "/glogs/";
      }
    } else {
      logFilePath_ = logFilePath;
    }
  }
}

int GlogLogger::setup() {
  int ec = MM_STATUS_OK;

  google::InitGoogleLogging(appId_.c_str());

  /* set log cleanup days */
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

  /* set log to file */
  if (logToFile_) {
    if (logLevelToFile_ == detail::LogLevel_NoLog) {
      std::fprintf(
          stderr, "Warning: File logging specified but log level is NoLog\n");
    } else {
      std::string dirPath = logFilePath_;

      // Ensure directory exists
      if (!createAbsDirectory(dirPath)) {
        std::fprintf(stderr, "Error: Failed to create log directory: %s\n",
            dirPath.c_str());
        return MM_STATUS_ENOENT;
      }

      // Ensure path ends with separator
      if (!dirPath.empty() && dirPath.back() != '/') {
        dirPath += '/';
      }

      std::string filePrefix = dirPath;
      // Define all possible log levels, in increasing severity
      const std::vector<std::pair<detail::LogLevel, int>> logLevelMap = {
          {detail::LogLevel_Debug, google::GLOG_INFO},
          {detail::LogLevel_Info, google::GLOG_INFO},
          {detail::LogLevel_Warn, google::GLOG_WARNING},
          {detail::LogLevel_Error, google::GLOG_ERROR},
          {detail::LogLevel_Fatal, google::GLOG_FATAL}};

      // Set log destination for each level
      for (const auto& [level, glogLevel] : logLevelMap) {
        // If configured level is less than or equal to this level, enable
        // logging for this level
        if (logLevelToFile_ <= level) {
          std::string levelPrefix;
          switch (glogLevel) {
            case google::GLOG_INFO: levelPrefix = filePrefix + "INFO."; break;
            case google::GLOG_WARNING:
              levelPrefix = filePrefix + "WARNING.";
              break;
            case google::GLOG_ERROR: levelPrefix = filePrefix + "ERROR."; break;
            case google::GLOG_FATAL: levelPrefix = filePrefix + "FATAL."; break;
            default: levelPrefix = filePrefix; break;
          }
          google::SetLogDestination(glogLevel, levelPrefix.c_str());
        } else {
          // For unwanted levels, set empty string to disable it
          google::SetLogDestination(glogLevel, "");
        }
      }
      // Set log symlinks
      google::SetLogSymlink(google::GLOG_INFO, appId_.c_str());
      google::SetLogSymlink(google::GLOG_WARNING, appId_.c_str());
      google::SetLogSymlink(google::GLOG_ERROR, appId_.c_str());
      google::SetLogSymlink(google::GLOG_FATAL, appId_.c_str());
    }
  } else {
    // If file logging is not enabled, explicitly disable all file output
    google::SetLogDestination(google::GLOG_INFO, "");
    google::SetLogDestination(google::GLOG_WARNING, "");
    google::SetLogDestination(google::GLOG_ERROR, "");
    google::SetLogDestination(google::GLOG_FATAL, "");
  }

  return ec;
}

int GlogLogger::teardown() {
  google::ShutdownGoogleLogging();
  return MM_STATUS_OK;
}

int GlogLogger::convertLogLevel(detail::LogLevel level) noexcept {
  int glogLevel = google::INFO;
  switch (level) {
    case detail::LogLevel_Debug: {
      glogLevel = google::INFO;
      break;
    }
    case detail::LogLevel_Info: {
      glogLevel = google::INFO;
      break;
    }
    case detail::LogLevel_Warn: {
      glogLevel = google::WARNING;
      break;
    }
    case detail::LogLevel_Error: {
      glogLevel = google::ERROR;
      break;
    }
    case detail::LogLevel_Fatal: {
      glogLevel = google::FATAL;
      break;
    }
    default: {
      break;
    }
  }

  return glogLevel;
}

void GlogLogger::logVerbose(const char* msg, const std::size_t len) {
  (void)(msg);
  (void)(len);
}

void GlogLogger::logDebug(const char* msg, const std::size_t len) {
  (void)(len);
  if (logDebugSwitch_)
    LOG(INFO) << msg;
  else
    (void)(msg);
}

void GlogLogger::logInfo(const char* msg, const std::size_t len) {
  (void)(len);
  LOG(INFO) << msg;
}

void GlogLogger::logWarn(const char* msg, const std::size_t len) {
  (void)(len);
  LOG(WARNING) << msg;
}

void GlogLogger::logError(const char* msg, const std::size_t len) {
  (void)(len);
  LOG(ERROR) << msg;
}

void GlogLogger::logFatal(const char* msg, const std::size_t len) {
  (void)(len);
  LOG(FATAL) << msg;
}

}  // namespace mm
