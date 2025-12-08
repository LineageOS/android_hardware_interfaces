/*
 * Copyright 2025 The Android Open Source Project
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

#pragma once

#include <string>

#include "bluetooth_hal/hal_types.h"

namespace bluetooth_hal {
namespace util {
namespace power {

class WakelockUtil {
 public:
  static std::string WakeSourceToString(WakeSource source) {
    switch (source) {
      case WakeSource::kTx:
        return "TX";
      case WakeSource::kRx:
        return "RX";
      case WakeSource::kHciBusy:
        return "HciBusy";
      case WakeSource::kRouterTask:
        return "RouterTask";
      case WakeSource::kTransport:
        return "Transport";
      case WakeSource::kInitialize:
        return "Initialize";
      case WakeSource::kClose:
        return "Close";
    }
    return "Unknown";
  }
};

}  // namespace power
}  // namespace util
}  // namespace bluetooth_hal
