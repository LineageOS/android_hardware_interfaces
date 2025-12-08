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

#include <cstdint>
#include <string>

#include "bluetooth_hal/util/android_base_wrapper.h"
#include "gmock/gmock.h"

namespace bluetooth_hal {
namespace util {

class MockAndroidBaseWrapper : public AndroidBaseWrapper {
 public:
  MOCK_METHOD(std::string, GetProperty,
              (const std::string& key, const std::string& default_value),
              (override));

  MOCK_METHOD(bool, GetBoolProperty,
              (const std::string& key, bool default_value), (override));

  MOCK_METHOD(bool, SetProperty,
              (const std::string& key, const std::string& value), (override));

  MOCK_METHOD(bool, ParseUint,
              (const std::string& s, uint8_t* out, uint8_t max), (override));

  static void SetMockWrapper(MockAndroidBaseWrapper* wrapper);

  static inline MockAndroidBaseWrapper* mock_android_base_wrapper_{nullptr};
};

}  // namespace util
}  // namespace bluetooth_hal
