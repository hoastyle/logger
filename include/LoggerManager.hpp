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

#ifndef INCLUDE_COMMON_LOG_LOGGERMANAGER_HPP_
#define INCLUDE_COMMON_LOG_LOGGERMANAGER_HPP_

#include "core/Singleton.hpp"
#include "log/DisallowCopy.hpp"
#include "log/ILogger.hpp"
#include "log/ILoggerFactory.hpp"
#include "log/LogBaseDef.hpp"
#include "util/Timer.hpp"
#include <ctime>
#include <time.h>

namespace mm {

class ILogger;
class ILoggerFactory;
class LoggerManager final : public CSingleton<LoggerManager> {
 public:
  LoggerManager() noexcept;
  ~LoggerManager() = default;

  void Start() noexcept;

  int setup(int argc, char* argv[]) noexcept;
  int teardown() noexcept;

  inline const LogConfig& config() const noexcept { return config_; }

  inline LoggerManagerPid pid() const noexcept { return pid_; }

  int setupLogger() noexcept;

 private:
  void initCmdLineFlags(const char* appId) noexcept;
  int parseCmdLineFlags(int argc, char* argv[]) noexcept;
  void checkLogConfig() noexcept;

  detail::LogLevel transCmdLevelToLogLevel(const char* cmdLevel) noexcept;
  detail::LogLevelCfg convertLogLevel() noexcept;
  void outputLog(const detail::LogLevel lvl, const char* const msg,
      const std::size_t len) noexcept;
  void sendErrorCodeHandler() noexcept;

  inline void logVerbose(const char* msg, const std::size_t len) noexcept {
    if (logger_) {
      logger_->logVerbose(msg, len);
    }
  }

  inline void logDebug(const char* msg, const std::size_t len) noexcept {
    if (logger_) {
      logger_->logDebug(msg, len);
    }
  }

  inline void logInfo(const char* msg, const std::size_t len) noexcept {
    if (logger_) {
      logger_->logInfo(msg, len);
    }
  }

  inline void logWarn(const char* msg, const std::size_t len) noexcept {
    if (logger_) {
      logger_->logWarn(msg, len);
    }
  }

  inline void logError(const char* msg, const std::size_t len) noexcept {
    if (logger_) {
      logger_->logError(msg, len);
    }
  }

  inline void logFatal(const char* msg, const std::size_t len) noexcept {
    if (logger_) {
      logger_->logFatal(msg, len);
    }
  }

  MM_DISALLOW_COPY_AND_MOVE(LoggerManager)

  LoggerManagerPid pid_;
  LogConfig config_;
  ILogger* logger_;
  ILoggerFactory* factory_;
};

}  // namespace mm

#endif  // INCLUDE_COMMON_LOG_LOGGERMANAGER_HPP_
