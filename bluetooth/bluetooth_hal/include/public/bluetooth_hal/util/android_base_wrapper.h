/*
 * Copyright 2024 The Android Open Source Project
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

class AndroidBaseWrapper {
 public:
  virtual ~AndroidBaseWrapper() = default;

  static AndroidBaseWrapper& GetWrapper();

  /**
   * @brief Retrieves a system property as a string.
   *
   * Given a property key, this function returns the corresponding value as a
   * string. If the property is not set, it returns the specified default value.
   *
   * @param key The property key to look up.
   * @param default_value The value to return if the property is not set.
   *
   * @return The property value as a string, or the default value if the
   * property is unset.
   *
   */
  virtual std::string GetProperty(const std::string& key,
                                  const std::string& default_value) = 0;

  /**
   * @brief Retrieves a system property as a boolean.
   *
   * Given a property key, this function returns the corresponding value as a
   * boolean. If the property is not set, it returns the specified default
   * boolean value.
   *
   * @param key The property key to look up.
   * @param default_value The boolean value to return if the property is not
   * set.
   *
   * @return The property value as a boolean, or the default value if the
   * property is unset.
   *
   */
  virtual bool GetBoolProperty(const std::string& key, bool default_value) = 0;

  /**
   * @brief Sets a system property to the specified string value.
   *
   * This function attempts to set the value of the system property identified
   * by the given key to the specified value. If the property cannot be set, it
   * returns `false`.
   *
   * @param key The property key to set.
   * @param value The value to set for the specified property key.
   *
   * @return `true` if the property was successfully set; otherwise, `false`.
   *
   */
  virtual bool SetProperty(const std::string& key,
                           const std::string& value) = 0;

  /**
   * @brief Parses an unsigned integer from a string.
   *
   * This function parses an unsigned integer from the given string.
   * The parsed value is stored in the `out` parameter.
   *
   * @param s The string to parse.
   * @param out A pointer to the variable where the parsed value will be stored.
   * @param max The maximum allowed value for the parsed integer.
   *
   * @return `true` if the parsing was successful, `false` otherwise.
   *
   */
  virtual bool ParseUint(const std::string& s, uint8_t* out, uint8_t max) = 0;
};

}  // namespace util
}  // namespace bluetooth_hal
