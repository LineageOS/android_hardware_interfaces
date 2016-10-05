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

#include "failure_reason_util.h"

using ::android::hardware::wifi::V1_0::CommandFailureReason;

namespace android {
namespace hardware {
namespace wifi {

std::string LegacyErrorToString(wifi_error error) {
  switch(error) {
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

V1_0::FailureReason CreateFailureReason(
    CommandFailureReason reason, const std::string& description) {
  V1_0::FailureReason result;
  result.reason = reason;
  result.description = description.data();
  return result;
}

V1_0::FailureReason CreateFailureReasonLegacyError(
    wifi_error error, const std::string& desc) {
  switch(error) {
    case WIFI_ERROR_UNINITIALIZED:
    case WIFI_ERROR_NOT_AVAILABLE:
      return CreateFailureReason(CommandFailureReason::NOT_AVAILABLE, desc);

    case WIFI_ERROR_NOT_SUPPORTED:
      return CreateFailureReason(CommandFailureReason::NOT_SUPPORTED, desc);

    case WIFI_ERROR_INVALID_ARGS:
    case WIFI_ERROR_INVALID_REQUEST_ID:
      return CreateFailureReason(CommandFailureReason::INVALID_ARGS, desc);

    case WIFI_ERROR_TIMED_OUT:
      return CreateFailureReason(
          CommandFailureReason::UNKNOWN, desc + ", timed out");

    case WIFI_ERROR_TOO_MANY_REQUESTS:
      return CreateFailureReason(
          CommandFailureReason::UNKNOWN, desc + ", too many requests");

    case WIFI_ERROR_OUT_OF_MEMORY:
      return CreateFailureReason(
          CommandFailureReason::UNKNOWN, desc + ", out of memory");

    case WIFI_ERROR_NONE:
    case WIFI_ERROR_UNKNOWN:
    default:
      return CreateFailureReason(CommandFailureReason::UNKNOWN, "unknown");
  }
}

}  // namespace wifi
}  // namespace hardware
}  // namespace android
