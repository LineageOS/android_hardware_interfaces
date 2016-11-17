/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "wifi_status_util.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {

std::string legacyErrorToString(wifi_error error) {
  switch (error) {
    case WIFI_SUCCESS:
      return "SUCCESS";
    case WIFI_ERROR_UNINITIALIZED:
      return "UNINITIALIZED";
    case WIFI_ERROR_NOT_AVAILABLE:
      return "NOT_AVAILABLE";
    case WIFI_ERROR_NOT_SUPPORTED:
      return "NOT_SUPPORTED";
    case WIFI_ERROR_INVALID_ARGS:
      return "INVALID_ARGS";
    case WIFI_ERROR_INVALID_REQUEST_ID:
      return "INVALID_REQUEST_ID";
    case WIFI_ERROR_TIMED_OUT:
      return "TIMED_OUT";
    case WIFI_ERROR_TOO_MANY_REQUESTS:
      return "TOO_MANY_REQUESTS";
    case WIFI_ERROR_OUT_OF_MEMORY:
      return "OUT_OF_MEMORY";
    case WIFI_ERROR_UNKNOWN:
    default:
      return "UNKNOWN";
  }
}

WifiStatus createWifiStatus(WifiStatusCode code,
                            const std::string& description) {
  WifiStatus result;
  result.code = code;
  result.description = description.data();
  return result;
}

WifiStatus createWifiStatus(WifiStatusCode code) {
  return createWifiStatus(code, "");
}

WifiStatus createWifiStatusFromLegacyError(wifi_error error,
                                           const std::string& desc) {
  switch (error) {
    case WIFI_ERROR_UNINITIALIZED:
    case WIFI_ERROR_NOT_AVAILABLE:
      return createWifiStatus(WifiStatusCode::ERROR_NOT_AVAILABLE, desc);

    case WIFI_ERROR_NOT_SUPPORTED:
      return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED, desc);

    case WIFI_ERROR_INVALID_ARGS:
    case WIFI_ERROR_INVALID_REQUEST_ID:
      return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS, desc);

    case WIFI_ERROR_TIMED_OUT:
      return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN,
                              desc + ", timed out");

    case WIFI_ERROR_TOO_MANY_REQUESTS:
      return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN,
                              desc + ", too many requests");

    case WIFI_ERROR_OUT_OF_MEMORY:
      return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN,
                              desc + ", out of memory");

    case WIFI_ERROR_NONE:
      return createWifiStatus(WifiStatusCode::SUCCESS, desc);

    case WIFI_ERROR_UNKNOWN:
    default:
      return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN, "unknown");
  }
}

WifiStatus createWifiStatusFromLegacyError(wifi_error error) {
  return createWifiStatusFromLegacyError(error, "");
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
