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

namespace bluetooth_hal {
namespace util {

class Logger {
 public:
  /**
   * @brief Generates a timestamp string suitable for log messages.
   *
   * The timestamp is formatted as HH:MM:SS:sss (hours, minutes, seconds,
   * milliseconds). It represents the current local time with millisecond
   * precision.
   *
   * @return A string representing the current local time in the format
   * HH:MM:SS:sss.
   *
   */
  static std::string GetLogFormatTimestamp();

  /**
   * @brief Generates a timestamp string suitable for record creation.
   *
   * The timestamp is formatted as YYYY-MM-DD_HH-MM-SS (year, month, day, hours,
   * minutes, seconds). It represents the current local time.
   *
   * @return A string representing the current local time in the format
   * YYYY-MM-DD_HH-MM-SS.
   *
   */
  static std::string GetFileFormatTimestamp();
};

}  // namespace util
}  // namespace bluetooth_hal
