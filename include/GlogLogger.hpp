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

#ifndef INCLUDE_COMMON_LOG_GLOGLOGGER_HPP_
#define INCLUDE_COMMON_LOG_GLOGLOGGER_HPP_

#include <glog/logging.h>
#include <stdio.h>
#include <stdlib.h>

#include "log/ILogger.hpp"

/* preprocess macro */
#ifdef COMPACT_GOOGLE_LOG_INFO
#undef COMPACT_GOOGLE_LOG_INFO
#define COMPACT_GOOGLE_LOG_INFO google::LogMessage("", 0)
#endif

#ifdef COMPACT_GOOGLE_LOG_WARNING
#undef COMPACT_GOOGLE_LOG_WARNING
#define COMPACT_GOOGLE_LOG_WARNING \
  google::LogMessage("", 0, google::GLOG_WARNING)
#endif

#ifdef COMPACT_GOOGLE_LOG_ERROR
#undef COMPACT_GOOGLE_LOG_ERROR
#define COMPACT_GOOGLE_LOG_ERROR google::LogMessage("", 0, google::GLOG_ERROR)
#endif

#ifdef COMPACT_GOOGLE_LOG_FATAL
#undef COMPACT_GOOGLE_LOG_FATAL
#define COMPACT_GOOGLE_LOG_FATAL google::LogMessage("", 0, google::GLOG_FATAL)
#endif

namespace mm {

const int GLOG_OVERDUE_DAY = 14;

class GlogLogger final : public ILogger {
 public:
  enum : LogToFile { DefaultLogToFile = false };
  enum : std::uint16_t { DefaultMaxPathSize = 256 };

  GlogLogger(const std::string& appId, const detail::LogLevel logLevelToStderr,
      const detail::LogLevel logLevelToFile, const LogToFile logToFile,
      const LogFilePath logFilePath,
      const LogDebugSwitch logDebugSwitch) noexcept;

  virtual ~GlogLogger() override = default;

  int setup() override;
  int teardown() override;

  virtual void logVerbose(const char* msg, const std::size_t len) override;
  virtual void logDebug(const char* msg, const std::size_t len) override;
  virtual void logInfo(const char* msg, const std::size_t len) override;
  virtual void logWarn(const char* msg, const std::size_t len) override;
  virtual void logError(const char* msg, const std::size_t len) override;
  virtual void logFatal(const char* msg, const std::size_t len) override;

 private:
  int convertLogLevel(detail::LogLevel level) noexcept;

  std::string appId_;
  detail::LogLevel logLevelToStderr_;
  detail::LogLevel logLevelToFile_;
  LogToFile logToFile_;
  LogFilePath logFilePath_;
  LogDebugSwitch logDebugSwitch_;
};

}  // namespace mm

#endif  // INCLUDE_COMMON_LOG_GLOGLOGGER_HPP_
