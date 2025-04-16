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

#include "log/Log.hpp"

#include <sys/time.h>
#include <jsoncpp/json/json.h>

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <vector>
#include <map>

#include "core/GlobalCommonConfig.hpp"
#include "basic/MMBasic.hpp"
#include "basic/MMErrCode.hpp"
#include "web/Producer.hpp"

namespace mm {

namespace detail {

enum { LogStackBufferSize = 2048 };
enum { LogTimeBufferSize = 64 };

static detail::LogCallback gLogCb     = nullptr;
static detail::LogLevelCfg gLogLvlCfg = detail::LogLevelCfg_NoLog;
static LogSinkType gLogSinkType       = LogSinkType_None;

void setupLogger(
    LogCallback&& cb, const LogLevelCfg cfg, const LogSinkType stype) noexcept {
  gLogCb       = std::move(cb);
  gLogLvlCfg   = cfg;
  gLogSinkType = stype;
}

void teardownLogger() noexcept {
  gLogLvlCfg   = detail::LogLevelCfg_NoLog;
  gLogSinkType = detail::LogSinkType_None;
  gLogCb       = nullptr;
}

static inline const char* getLogLvlString(const detail::LogLevel lvl) {
  const char* ret = nullptr;
  switch (lvl) {
    case detail::LogLevel_Verbose: {
      ret = "Verbose";
      break;
    }
    case detail::LogLevel_Debug: {
      ret = "Debug";
      break;
    }
    case detail::LogLevel_Info: {
      ret = "Info";
      break;
    }
    case detail::LogLevel_Warn: {
      ret = "Warn";
      break;
    }
    case detail::LogLevel_Error: {
      ret = "Error";
      break;
    }
    case detail::LogLevel_Fatal: {
      ret = "Fatal";
      break;
    }
    default: {
      ret = "Unknown";
      break;
    }
  }

  return ret;
}

void outputLog(const detail::LogLevel lvl, const char* const filename,
    const char* const funcname, const int line, const char* const fmt,
    ...) noexcept {
  char buf[detail::LogStackBufferSize];
  int n      = 0;
  int offset = 0;
  int len    = detail::LogStackBufferSize;

  bool checked = false;
  if (detail::LogSinkType_Stdout == gLogSinkType) {
    if (gLogLvlCfg & lvl) {
      checked = true;
      /* append timestamp. */
      struct timeval tv;
      char timebuf[LogTimeBufferSize];

      ::gettimeofday(&tv, NULL);
      ::strftime(timebuf, sizeof(timebuf) - 1, "%Y-%m-%d %H:%M:%S",
          localtime(&tv.tv_sec));

      n = std::snprintf(buf + offset, static_cast<std::size_t>(len),
          "%s.%03d %05ld", timebuf, (int)(tv.tv_usec / 1000), gettid());

      if (0 > n) {
        /* there is an error occurred std::snprintf(). */
        return;
      }

      if (n >= len) {
        /* truncated already, do ouput.*/
        if (gLogCb) {
          gLogCb(lvl, buf, detail::LogStackBufferSize);
        }

        return;
      }

      offset += n;
      len -= n;
    }
  } else {
    checked = true;
  }

  if (checked) {
    /* append log postion. */
    const char* lvlStr = getLogLvlString(lvl);

    /**
     * Extract the main strings of the filename and function name, i.e.
     *                        MNode::bindNode()
     *           DeviceManager::DeviceManager()
     * YolactObjectDetect::YolactObjectDet...()
     */
    const int filenameLenMax = 18;
    const int funcnameLenMax = 18;

    char body[filenameLenMax + 2 + funcnameLenMax + 2 + 1] = {0};

    // remove the suffix, such as ".cpp"
    char agent[filenameLenMax * 2 + 1] = {0};
    strncpy(agent, filename, filenameLenMax * 2);
    char* pFilename = strtok(agent, ".");
    if (NULL == pFilename) pFilename = agent;

    if ((strlen(pFilename) + strlen(funcname)) <=
        (filenameLenMax + funcnameLenMax)) {
      strcat(body, pFilename);
      strcat(body, "::");
      strcat(body, funcname);
      strcat(body, "()");
    } else {
      int filenameLenCopied = filenameLenMax;
      if (strlen(funcname) < funcnameLenMax) {
        filenameLenCopied += funcnameLenMax - strlen(funcname);
      }
      strncpy(body, pFilename, filenameLenCopied);

      strcat(body, "::");
      int bodyLen = strlen(body);
      strncpy(body + bodyLen, funcname,
          filenameLenMax + 2 + funcnameLenMax - bodyLen);

      if ((bodyLen + strlen(funcname)) >
          (filenameLenMax + 2 + funcnameLenMax)) {
        body[filenameLenMax + 2 + funcnameLenMax - 1] = '.';
        body[filenameLenMax + 2 + funcnameLenMax - 2] = '.';
        body[filenameLenMax + 2 + funcnameLenMax - 3] = '.';
      }
      strcat(body, "()");
    }

    n = std::snprintf(buf + offset, static_cast<std::size_t>(len),
        " %40.40s %04d %c: ", body, line, lvlStr[0]);

    if (0 > n) {
      return;
    }

    if (n >= len) {
      if (gLogCb) {
        gLogCb(lvl, buf, detail::LogStackBufferSize);
      }

      return;
    }

    offset += n;
    len -= n;

    /* append format. */
    va_list args;
    va_start(args, fmt);
    n = std::vsnprintf(buf + offset, static_cast<std::size_t>(len), fmt, args);
    if (0 > n) {
      return;
    }

    if (n >= len) {
      if (gLogCb) {
        gLogCb(lvl, buf, detail::LogStackBufferSize);
      }

      return;
    }

    offset += n;
    len -= n;

    va_end(args);

    /* do final output. */
    if (gLogCb) {
      gLogCb(lvl, buf, offset + 1);
    }
  }
}

std::string convertOutputLogToStr(const detail::LogLevel lvl,
    const char* const filename, const char* const funcname, const int line,
    const char* const fmt, ...) noexcept {
  char buf[detail::LogStackBufferSize];
  int n      = 0;
  int offset = 0;
  int len    = detail::LogStackBufferSize;

  /* append log postion. */
  const char* lvlStr = getLogLvlString(lvl);

  /**
   * Extract the main strings of the filename and function name, i.e.
   *                        MNode::bindNode()
   *           DeviceManager::DeviceManager()
   * YolactObjectDetect::YolactObjectDet...()
   */
  const int filenameLenMax = 18;
  const int funcnameLenMax = 18;

  char body[filenameLenMax + 2 + funcnameLenMax + 2 + 1] = {0};

  // remove the suffix, such as ".cpp"
  char agent[filenameLenMax * 2 + 1] = {0};
  strncpy(agent, filename, filenameLenMax * 2);
  char* pFilename = strtok(agent, ".");
  if (NULL == pFilename) pFilename = agent;

  if ((strlen(pFilename) + strlen(funcname)) <=
      (filenameLenMax + funcnameLenMax)) {
    strcat(body, pFilename);
    strcat(body, "::");
    strcat(body, funcname);
    strcat(body, "()");
  } else {
    int filenameLenCopied = filenameLenMax;
    if (strlen(funcname) < funcnameLenMax) {
      filenameLenCopied += funcnameLenMax - strlen(funcname);
    }
    strncpy(body, pFilename, filenameLenCopied);

    strcat(body, "::");
    int bodyLen = strlen(body);
    strncpy(body + bodyLen, funcname,
        filenameLenMax + 2 + funcnameLenMax - bodyLen);

    if ((bodyLen + strlen(funcname)) > (filenameLenMax + 2 + funcnameLenMax)) {
      body[filenameLenMax + 2 + funcnameLenMax - 1] = '.';
      body[filenameLenMax + 2 + funcnameLenMax - 2] = '.';
      body[filenameLenMax + 2 + funcnameLenMax - 3] = '.';
    }
    strcat(body, "()");
  }

  n = std::snprintf(buf + offset, static_cast<std::size_t>(len),
      " %40.40s %04d %c: ", body, line, lvlStr[0]);

  if (0 > n) {
    return "";
  }

  if (n >= len) {
    std::string ret = buf;
    return ret;
  }

  offset += n;
  len -= n;

  /* append format. */

  va_list args;
  va_start(args, fmt);
  n = std::vsnprintf(buf + offset, static_cast<std::size_t>(len), fmt, args);

  if (0 > n) {
    return "";
  }

  if (n >= len) {
    std::string ret = buf;
    return ret;
  }

  offset += n;
  len -= n;
  va_end(args);

  std::string ret = buf;
  return ret;
}

}  // namespace detail

void usage(int ecode) noexcept {
  fprintf(stderr,
      "mCrane [options][parameter]: description\n"
      "  [--aplc]=<on/off>: options for open/close plc module\n"
      "  [--appid]: set current proc name with appid\n"
      "  [--coredump]=<on/off>: options for open/close coredump\n"
      "  [--debugSwitch]: true/false, enable/disable MM_DEBUG\n"
      "  [--file]=<true|false>: options for open/close log file mode\n"
      "  [--filepath]: set log output file path\n"
      "  [--help|-h|-?]: check cmdline parameters options\n"
      "  [--sim]: options for open simulation with path\n"
      "  [--sinktype]=<Stdout|GLog>: options for logging protocol\n"
      "  [--toFile]=<verbose|debug|info|warn|error|fatal>: log level\n"
      "  [--toTerm]=<verbose|debug|info|warn|error|fatal>: log level\n");
  exit(ecode);
}

bool isFileExist(const std::string& path) noexcept {
  return !access(path.c_str(), F_OK);
}

bool createAbsDirectory(const std::string& directoryPath) noexcept {
  bool ret = true;

  // #define MAX_PATH_LEN 256
  const int MAX_PATH_LEN = 256;
  uint32_t dirPathLen    = directoryPath.length();
  if (MAX_PATH_LEN < dirPathLen) {
    ret = false;
    return ret;
  }

  char tmpDirPath[MAX_PATH_LEN] = {0};
  for (uint32_t i = 0; i < dirPathLen; ++i) {
    tmpDirPath[i] = directoryPath[i];
    if (tmpDirPath[i] == '/') {
      if (!isFileExist(tmpDirPath)) {
        int status = mkdir(tmpDirPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (0 != status) {
          ret = false;
          break;
        }
      }
    }
  }

  return ret;
}

bool noError(int ec) noexcept { return (MM_STATUS_OK == ec); }

}  // namespace mm
