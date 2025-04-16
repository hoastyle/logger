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
    const LogFilePath logFilePath, const LogDebugSwitch logDebugSwitch) noexcept
    : appId_(appId),
      logLevelToStderr_(logLevelToStderr),
      logLevelToFile_(logLevelToFile),
      logToFile_(logToFile),
      logFilePath_(logFilePath),
      logDebugSwitch_(logDebugSwitch) {
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
  /* set log to stderr */
  google::SetStderrLogging(convertLogLevel(logLevelToStderr_));
  /* set color */
  FLAGS_colorlogtostderr = true;

  /* set log to file */
  if (logToFile_) {
    if (createAbsDirectory(logFilePath_)) {
      for (auto logLevel = logLevelToFile_;
           logLevel <= detail::LogLevel::LogLevel_Fatal;
           logLevel = detail::LogLevel(logLevel << 1))
        google::SetLogDestination(
            convertLogLevel(logLevel), logFilePath_.c_str());
    } else {
      ec = MM_STATUS_ENOENT;
      LOG(ERROR) << "Failed to create log file: " << logFilePath_;
    }
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
