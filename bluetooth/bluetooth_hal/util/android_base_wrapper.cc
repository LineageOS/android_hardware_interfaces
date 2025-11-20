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

#include "bluetooth_hal/util/android_base_wrapper.h"

#include <string>

#include "android-base/parseint.h"
#include "android-base/properties.h"

namespace bluetooth_hal {
namespace util {

class AndroidBaseWrapperImpl : public AndroidBaseWrapper {
 public:
  std::string GetProperty(const std::string& key,
                          const std::string& default_value) override {
    return ::android::base::GetProperty(key, default_value);
  }

  bool GetBoolProperty(const std::string& key, bool default_value) override {
    return ::android::base::GetBoolProperty(key, default_value);
  }

  bool SetProperty(const std::string& key, const std::string& value) override {
    return ::android::base::SetProperty(key, value);
  }

  bool ParseUint(const std::string& s, uint8_t* out, uint8_t max) override {
    return ::android::base::ParseUint<uint8_t>(s, out, max);
  }
};

AndroidBaseWrapper& AndroidBaseWrapper::GetWrapper() {
  static AndroidBaseWrapperImpl wrapper;
  return wrapper;
}

}  // namespace util
}  // namespace bluetooth_hal
