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

#ifndef INCLUDE_COMMON_LOG_ILOGGERFACTORY_HPP_
#define INCLUDE_COMMON_LOG_ILOGGERFACTORY_HPP_

#include "LogBaseDef.hpp"

namespace mm {

class ILogger;
class ILoggerFactory {
 public:
  ILoggerFactory() noexcept = default;
  virtual ~ILoggerFactory() = default;

  virtual ILogger* createLogger(const LogConfig& config) = 0;
  virtual void destroyLogger(ILogger* p)                 = 0;
};

}  // namespace mm

#endif  // INCLUDE_COMMON_LOG_ILOGGERFACTORY_HPP_
