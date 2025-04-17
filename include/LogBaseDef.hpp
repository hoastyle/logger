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

#ifndef INCLUDE_COMMON_LOG_LOGBASEDEF_HPP_
#define INCLUDE_COMMON_LOG_LOGBASEDEF_HPP_

#include <chrono>
#include <functional>
#include <string>

namespace mm {

namespace detail {

using LogLevelCfg = std::uint32_t;

enum LogLevel : detail::LogLevelCfg {
  /* specific value for no log. */
  LogLevel_NoLog = 0,
  /* normal log levels. */
  LogLevel_Verbose = 1,
  LogLevel_Debug   = 1 << 1,
  LogLevel_Info    = 1 << 2,
  LogLevel_Warn    = 1 << 3,
  LogLevel_Error   = 1 << 4,
  LogLevel_Fatal   = 1 << 5,
};

enum : LogLevelCfg {
  LogLevelCfg_NoLog   = detail::LogLevel_NoLog,
  LogLevelCfg_Fatal   = detail::LogLevelCfg_NoLog | detail::LogLevel_Fatal,
  LogLevelCfg_Error   = detail::LogLevelCfg_Fatal | detail::LogLevel_Error,
  LogLevelCfg_Warn    = detail::LogLevelCfg_Error | detail::LogLevel_Warn,
  LogLevelCfg_Info    = detail::LogLevelCfg_Warn | detail::LogLevel_Info,
  LogLevelCfg_Debug   = detail::LogLevelCfg_Info | detail::LogLevel_Debug,
  LogLevelCfg_Verbose = detail::LogLevelCfg_Debug | detail::LogLevel_Verbose,
};

enum LogSinkType : std::uint8_t {
  LogSinkType_None = 0u,
  LogSinkType_Stdout,
  LogSinkType_GLog,
  LogSinkType_OptimizedGLog,
};

using LogCallback = std::function<void(
    const detail::LogLevel, const char* const, const std::size_t)>;

}  // namespace detail

using LogToFile        = bool;
using LogDebugSwitch   = bool;
using LogFilePath      = std::string;
using LoggerManagerPid = pid_t;

// Add additional configuration options for OptimizedGlogLogger
struct LoggerOptimizationConfig {
  LoggerOptimizationConfig() noexcept
      : batchSize(100), queueCapacity(10000), numWorkers(2), poolSize(10000) {}

  size_t batchSize;      // Number of messages to process in a batch
  size_t queueCapacity;  // Maximum queue size before dropping messages
  size_t numWorkers;     // Number of worker threads
  size_t poolSize;       // Size of the memory pool
};

struct LogConfig {
  LogConfig() noexcept
      : appId_(),
        logLevelToStderr_(detail::LogLevel_Info),
        logLevelToFile_(detail::LogLevel_NoLog),
        logSinkType_(detail::LogSinkType_Stdout),
        logToFile_(false),
        logFilePath_(),
        logDebugSwitch_(false),
        logToConsole_(false),
        optimizationConfig_() {}

  virtual ~LogConfig() = default;

  std::string appId_;
  detail::LogLevel logLevelToStderr_;
  detail::LogLevel logLevelToFile_;
  detail::LogSinkType logSinkType_;
  mm::LogToFile logToFile_;
  mm::LogFilePath logFilePath_;
  mm::LogDebugSwitch logDebugSwitch_;
  bool logToConsole_;  // New option to control console output
  LoggerOptimizationConfig optimizationConfig_;
};

}  // namespace mm

#endif  // INCLUDE_COMMON_LOG_LOGBASEDEF_HPP_
