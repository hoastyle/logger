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

#include "log/LoggerManager.hpp"

#include <unistd.h>

#include <cstring>

#include "log/Log.hpp"
#include "log/LoggerFactory.hpp"

namespace mm {

LoggerManager::LoggerManager() noexcept
    : pid_(), config_(), logger_(nullptr), factory_(nullptr) {
  pid_ = ::getpid();
}

void LoggerManager::Start() noexcept {}

void LoggerManager::initCmdLineFlags(const char* appId) noexcept {
  if (!appId) {
    config_.appId_ = "";
  } else {
    config_.appId_ = appId;
  }

  config_.logLevelToStderr_ = detail::LogLevel_Debug;
  config_.logLevelToFile_   = detail::LogLevel_NoLog;
  config_.logSinkType_      = detail::LogSinkType_Stdout;
  config_.logToFile_        = false;
  config_.logFilePath_      = "";
  config_.logDebugSwitch_   = false;
}

int LoggerManager::setup(int argc, char* argv[]) noexcept {
  int ec = MM_STATUS_OK;

  do {
    ec = parseCmdLineFlags(argc, argv);
    if (noError(ec)) {
      ec = MM_STATUS_ERROR;
    }

    factory_ = new (std::nothrow) LoggerFactory(config_.appId_);
    if (!factory_) {
      ec = MM_STATUS_ENOMEM;
      break;
    }

    if (detail::LogLevel_NoLog != config_.logLevelToStderr_) {
      logger_ = factory_->createLogger(config_);
      if (!logger_) {
        ec = MM_STATUS_ENOMEM;
        break;
      }

      ec = logger_->setup();
      if (!noError(ec)) {
        break;
      }
    }
  } while (0);

  return ec;
}

int LoggerManager::setupLogger() noexcept {
  detail::LogLevelCfg logLvlConfig = convertLogLevel();
  detail::LogCallback logCallback  = std::bind(&LoggerManager::outputLog, this,
      std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

  detail::setupLogger(
      std::move(logCallback), logLvlConfig, config_.logSinkType_);

  return MM_STATUS_OK;
}

int LoggerManager::teardown() noexcept {
  int ec = MM_STATUS_OK;

  if (logger_) {
    logger_->teardown();
    delete logger_;
    logger_ = nullptr;
  }

  if (factory_) {
    delete factory_;
    factory_ = nullptr;
  }

  return ec;
}

int LoggerManager::parseCmdLineFlags(int argc,
    char* argv[]) noexcept  //! OCLINT
{
  initCmdLineFlags(argv[0]);
  int ec = MM_STATUS_OK;
  int i  = 1;
  while (i < argc) {
    const char* arg = argv[i];
    if (strstr(arg, "--toTerm=") == arg) {
      if (*(strchr(arg, '=') + 1) == '\0') {
        fprintf(stderr, "\"--toTerm=\" requires a log level\n");
        usage(1);
      }

      const char* cmdLevel      = strchr(arg, '=') + 1;
      config_.logLevelToStderr_ = transCmdLevelToLogLevel(cmdLevel);
    } else if (strstr(arg, "--toFile=") == arg) {
      if (*(strchr(arg, '=') + 1) == '\0') {
        fprintf(stderr, "\"--toFile=\" requires a log level\n");
        usage(1);
      }

      const char* cmdLevel    = strchr(arg, '=') + 1;
      config_.logLevelToFile_ = transCmdLevelToLogLevel(cmdLevel);
    } else if (strstr(arg, "--sinktype=") == arg) {
      if (*(strchr(arg, '=') + 1) == '\0') {
        fprintf(stderr, "\"--sinktype=\" requires a log sinktype\n");
        usage(1);
      }

      const char* sinktype = strchr(arg, '=') + 1;

      if (strcmp(sinktype, "GLog") == 0) {
        config_.logSinkType_ = detail::LogSinkType::LogSinkType_GLog;
      } else if (strcmp(sinktype, "Stdout") == 0) {
        config_.logSinkType_ = detail::LogSinkType::LogSinkType_Stdout;
      } else {
        fprintf(stderr, "sinktype value %s is invalid!\n", sinktype);
        usage(1);
      }
    } else if (strstr(arg, "--file=") == arg) {
      if (*(strchr(arg, '=') + 1) == '\0') {
        fprintf(stderr, "\"--file=\" requires an file val\n");
        usage(1);
      }
      const char* logtofile = strchr(arg, '=') + 1;
      if ((strcmp(logtofile, "true") == 0) ||
          (strcmp(logtofile, "TRUE") == 0)) {
        config_.logToFile_ = true;
      } else if ((strcmp(logtofile, "false") == 0) ||
                 (strcmp(logtofile, "FALSE") == 0)) {
        config_.logToFile_ = false;
      } else {
        fprintf(stderr, "logtofile value %s is invalid!\n", logtofile);
        usage(1);
      }
    } else if (strstr(arg, "--filepath=") == arg) {
      if (*(strchr(arg, '=') + 1) == '\0') {
        fprintf(stderr, "\"--filepath=\" requires a filepath\n");
        usage(1);
      }

      const char* logfilepath = strchr(arg, '=') + 1;
      config_.logFilePath_    = logfilepath;
    } else if (strstr(arg, "--appid=") == arg) {
      if (*(strchr(arg, '=') + 1) == '\0') {
        fprintf(stderr, "\"--appid=\" requires a appid\n");
        usage(1);
      }

      const char* appid = strchr(arg, '=') + 1;
      config_.appId_    = appid;
    } else if (strstr(arg, "--debugSwitch=") == arg) {
      if (*(strchr(arg, '=') + 1) == '\0') {
        fprintf(stderr, "\"--debugSwitch=\" requires switch signal\n");
        usage(1);
      }

      const char* logDebugSwitch = strchr(arg, '=') + 1;
      if ((strcmp(logDebugSwitch, "true") == 0) ||
          (strcmp(logDebugSwitch, "TRUE") == 0)) {
        config_.logDebugSwitch_ = true;
      } else if ((strcmp(logDebugSwitch, "false") == 0) ||
                 (strcmp(logDebugSwitch, "FALSE") == 0)) {
        config_.logDebugSwitch_ = false;
      } else {
        fprintf(
            stderr, "logDebugSwitch value %s is invalid!\n", logDebugSwitch);
        usage(1);
      }

    } else {
      /* do nothing */
    }
    i++;
  }

  checkLogConfig();

  return ec;
}

void LoggerManager::checkLogConfig() noexcept {
  if (detail::LogSinkType::LogSinkType_Stdout == config_.logSinkType_) {
    if ((true == config_.logToFile_) ||
        (detail::LogLevel_NoLog != config_.logLevelToFile_) ||
        (!config_.logFilePath_.empty())) {
      fprintf(
          stderr, "icrane: log sink type stdout can not support log to file\n");
      exit(1);
    }
  }

  if (detail::LogSinkType::LogSinkType_GLog == config_.logSinkType_) {
    if ((detail::LogLevel_Verbose == config_.logLevelToStderr_) ||
        (detail::LogLevel_Verbose == config_.logLevelToFile_)) {
      fprintf(stderr,
          "icrane: log sink type glog only support log level "
          "debug|info|warn|error|fatal\n");
      exit(1);
    }

    if (!config_.logToFile_ &&
        ((detail::LogLevel_NoLog != config_.logLevelToFile_) ||
            (!config_.logFilePath_.empty()))) {
      fprintf(stderr,
          "icrane: need to set [--file] option before openning file mode\n");
      exit(1);
    }
  }
}
detail::LogLevel LoggerManager::transCmdLevelToLogLevel(
    const char* cmdLevel) noexcept {
  detail::LogLevel ret = detail::LogLevel_Info;

  if ((strcmp(cmdLevel, "verbose") == 0) ||
      (strcmp(cmdLevel, "VERBOSE") == 0)) {
    ret = detail::LogLevel_Verbose;
  } else if ((strcmp(cmdLevel, "debug") == 0) ||
             (strcmp(cmdLevel, "DEBUG") == 0)) {
    ret = detail::LogLevel_Debug;
  } else if ((strcmp(cmdLevel, "info") == 0) ||
             (strcmp(cmdLevel, "INFO") == 0)) {
    ret = detail::LogLevel_Info;
  } else if ((strcmp(cmdLevel, "warn") == 0) ||
             (strcmp(cmdLevel, "WARN") == 0)) {
    ret = detail::LogLevel_Warn;
  } else if ((strcmp(cmdLevel, "error") == 0) ||
             (strcmp(cmdLevel, "ERROR") == 0)) {
    ret = detail::LogLevel_Error;
  } else if ((strcmp(cmdLevel, "fatal") == 0) ||
             (strcmp(cmdLevel, "FATAL") == 0)) {
    ret = detail::LogLevel_Fatal;
  } else {
    fprintf(stderr, "loglevel value %s is invalid!\n", cmdLevel);
    usage(1);
  }

  return ret;
}

detail::LogLevelCfg LoggerManager::convertLogLevel() noexcept {
  detail::LogLevelCfg ret = detail::LogLevel_NoLog;

  switch (config_.logLevelToStderr_) {
    case detail::LogLevel_Verbose: {
      ret = detail::LogLevelCfg_Verbose;
      break;
    }
    case detail::LogLevel_Debug: {
      ret = detail::LogLevelCfg_Debug;
      break;
    }
    case detail::LogLevel_Info: {
      ret = detail::LogLevelCfg_Info;
      break;
    }
    case detail::LogLevel_Warn: {
      ret = detail::LogLevelCfg_Warn;
      break;
    }
    case detail::LogLevel_Error: {
      ret = detail::LogLevelCfg_Error;
      break;
    }
    case detail::LogLevel_Fatal: {
      ret = detail::LogLevelCfg_Fatal;
      break;
    }
    default: {
      break;
    }
  }

  return ret;
}

void LoggerManager::outputLog(const detail::LogLevel lvl, const char* const msg,
    const std::size_t len) noexcept {
  switch (lvl) {
    case detail::LogLevel_Verbose: logVerbose(msg, len); break;
    case detail::LogLevel_Debug: logDebug(msg, len); break;
    case detail::LogLevel_Info: logInfo(msg, len); break;
    case detail::LogLevel_Warn: logWarn(msg, len); break;
    case detail::LogLevel_Error: logError(msg, len); break;
    case detail::LogLevel_Fatal: logFatal(msg, len); break;
    default: break;
  }
}

}  // namespace mm
