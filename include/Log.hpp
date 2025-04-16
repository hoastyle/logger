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

#ifndef INCLUDE_COMMON_LOG_LOG_HPP_
#define INCLUDE_COMMON_LOG_LOG_HPP_

#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <cstring>
#include <unordered_map>
#include <string>

#include "LogBaseDef.hpp"

namespace mm {

namespace detail {

static const int64_t mTimeStampDelta = 1000;
static std::unordered_map<std::string, int64_t> mmKeyTimeStamp;

void setupLogger(detail::LogCallback&& cb, const detail::LogLevelCfg cfg,
    const LogSinkType stype) noexcept;

void teardownLogger() noexcept;

void outputLog(const detail::LogLevel lvl, const char* const file,
    const char* const func, const int line, const char* const fmt,
    ...) noexcept;

std::string convertOutputLogToStr(const detail::LogLevel lvl,
    const char* const file, const char* const func, const int line,
    const char* const fmt, ...) noexcept;

inline std::string getTimeString() {
  auto curTime =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  char currentTime[64];
  strftime(currentTime, sizeof(currentTime), "%Y-%m-%d %H:%M:%S",
      localtime(&curTime));
  return std::string(currentTime);
}

}  // namespace detail

void usage(int ecode) noexcept;
bool isFileExist(const std::string& path) noexcept;
bool createAbsDirectory(const std::string& directoryPath) noexcept;
bool noError(int ec) noexcept;

}  // namespace mm

// Get the file name only. __FILE__ returns the path + the name.
#define __file__ \
  (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#ifdef MM_ENABLE_LOGGING

#define MM_VERBOSE(fmt, ...)                                              \
  mm::detail::outputLog(mm::detail::LogLevel_Verbose, __file__, __func__, \
      __LINE__, fmt, ##__VA_ARGS__)

#ifdef MM_ENABLE_DEBUG
#define MM_DEBUG(fmt, ...)                                              \
  mm::detail::outputLog(mm::detail::LogLevel_Debug, __file__, __func__, \
      __LINE__, fmt, ##__VA_ARGS__)
#else
#define MM_DEBUG(fmt, ...)
#endif

#define MM_INFO(fmt, ...)                                              \
  mm::detail::outputLog(mm::detail::LogLevel_Info, __file__, __func__, \
      __LINE__, fmt, ##__VA_ARGS__)

#define MM_WARN(fmt, ...)                                              \
  mm::detail::outputLog(mm::detail::LogLevel_Warn, __file__, __func__, \
      __LINE__, fmt, ##__VA_ARGS__)

#define MM_ERROR(fmt, ...)                                              \
  mm::detail::outputLog(mm::detail::LogLevel_Error, __file__, __func__, \
      __LINE__, fmt, ##__VA_ARGS__)

#define MM_FATAL(fmt, ...)                                              \
  mm::detail::outputLog(mm::detail::LogLevel_Fatal, __file__, __func__, \
      __LINE__, fmt, ##__VA_ARGS__)

#else

#define MM_VERBOSE(fmt, ...)
#define MM_DEBUG(fmt, ...)
#define MM_INFO(fmt, ...)
#define MM_WARN(fmt, ...)
#define MM_ERROR(fmt, ...)
#define MM_FATAL(fmt, ...)

#endif

namespace mm {
namespace detail {

class RateLimitedLog {
 private:
  std::chrono::milliseconds interval_;
  std::chrono::steady_clock::time_point last_log_time_;

 public:
  RateLimitedLog(std::chrono::milliseconds interval)
      : interval_(interval), last_log_time_(std::chrono::steady_clock::now()) {}

  template <typename... Args>
  void Log(mm::detail::LogLevel severity, const char* format, Args... args) {
    auto now         = std::chrono::steady_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - last_log_time_);
    if (duration_ms >= interval_) {
      switch (severity) {
        case mm::detail::LogLevel_Debug: MM_DEBUG(format, args...); break;
        case mm::detail::LogLevel_Info: MM_INFO(format, args...); break;
        case mm::detail::LogLevel_Warn: MM_WARN(format, args...); break;
        case mm::detail::LogLevel_Error: MM_ERROR(format, args...); break;
        case mm::detail::LogLevel_Fatal: MM_FATAL(format, args...); break;
        default: MM_INFO(format, args...);
      }
      last_log_time_ = now;
    }
  }

  bool noRateLimited() {
    auto now         = std::chrono::steady_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - last_log_time_);
    if (duration_ms >= interval_) {
      last_log_time_ = now;
      return true;
    }
    return false;
  }
};
};  // namespace detail
}  // namespace mm

#endif  // INCLUDE_COMMON_LOG_LOG_HPP_
