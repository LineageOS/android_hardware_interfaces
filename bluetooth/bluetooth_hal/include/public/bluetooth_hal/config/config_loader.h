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
#include <string_view>

namespace bluetooth_hal {
namespace config {

class ConfigLoader {
 public:
  virtual ~ConfigLoader() = default;

  /**
   * @brief Loads the configuration using a custom-defined mechanism.
   *
   * This is a pure virtual function that must be implemented by all
   * derived classes. It serves as the primary entry point for
   * configuration loading, and can internally invoke other helper
   * methods such as LoadConfigFromFile or LoadConfigFromString.
   *
   * @return true if configuration is loaded successfully, false otherwise.
   */
  virtual bool LoadConfig() = 0;

  /**
   * @brief Loads the configuration from a file.
   *
   * This function provides a default implementation for loading configuration
   * from a given file path. Derived classes may override this method to provide
   * specific file-based loading behavior if needed.
   *
   * @param path The path to the file to be read for loading the configuration.
   * @return true if the configuration is successfully loaded from the file,
   * false otherwise.
   */
  virtual bool LoadConfigFromFile(std::string_view path) {
    (void)path;
    return false;
  }

  /**
   * @brief Loads the configuration from a string.
   *
   * This function provides a default implementation for loading configuration
   * from a raw string (e.g., JSON content). Derived classes may override this
   * method to support loading from string-based sources.
   *
   * @param content The string content to parse and load the configuration
   * from.
   * @return true if the configuration is successfully parsed and loaded, false
   * otherwise.
   */
  virtual bool LoadConfigFromString(std::string_view content) {
    (void)content;
    return false;
  }

  /**
   * @brief Dumps the current configuration state into a string.
   *
   * This pure virtual function must be implemented by derived classes to
   * provide a string representation of their loaded configuration.
   * @return A string containing the current configuration state.
   */
  virtual std::string DumpConfigToString() const = 0;
};

}  // namespace config
}  // namespace bluetooth_hal
