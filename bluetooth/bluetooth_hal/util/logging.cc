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

#include "bluetooth_hal/util/logging.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

namespace bluetooth_hal {
namespace util {
namespace {

// Helper function to format time with leading zeros.
std::string FormatTimeComponent(int value, int width) {
  std::stringstream ss;
  ss << std::setw(width) << std::setfill('0') << value;
  return ss.str();
}

}  // namespace

std::string Logger::GetLogFormatTimestamp() {
  const auto now = std::chrono::system_clock::now();
  const auto now_ms =
      std::chrono::time_point_cast<std::chrono::milliseconds>(now);
  const std::chrono::duration value = now_ms.time_since_epoch();

  // Get seconds and milliseconds separately.
  const long long seconds =
      std::chrono::duration_cast<std::chrono::seconds>(value).count();
  const long long milliseconds = value.count() % 1000;

  // Convert to local time.
  const std::time_t now_c = static_cast<std::time_t>(seconds);
  const std::tm* now_tm = std::localtime(&now_c);

  std::stringstream ss;
  ss << FormatTimeComponent(now_tm->tm_hour, 2) << ":"
     << FormatTimeComponent(now_tm->tm_min, 2) << ":"
     << FormatTimeComponent(now_tm->tm_sec, 2) << ":"
     << FormatTimeComponent(static_cast<int>(milliseconds), 3);

  return ss.str();
}

std::string Logger::GetFileFormatTimestamp() {
  const auto now = std::chrono::system_clock::now();
  const std::time_t now_c = std::chrono::system_clock::to_time_t(now);

  // Convert to local time.
  const std::tm* now_tm = std::localtime(&now_c);

  std::stringstream ss;
  ss << FormatTimeComponent(now_tm->tm_year + 1900, 4) << "-"
     << FormatTimeComponent(now_tm->tm_mon + 1, 2) << "-"
     << FormatTimeComponent(now_tm->tm_mday, 2) << "_"
     << FormatTimeComponent(now_tm->tm_hour, 2) << "-"
     << FormatTimeComponent(now_tm->tm_min, 2) << "-"
     << FormatTimeComponent(now_tm->tm_sec, 2);

  return ss.str();
}

}  // namespace util
}  // namespace bluetooth_hal
