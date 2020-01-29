/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <aidl/android/hardware/rebootescrow/BnRebootEscrow.h>

namespace aidl {
namespace android {
namespace hardware {
namespace rebootescrow {

class RebootEscrow : public BnRebootEscrow {
  public:
    explicit RebootEscrow(const std::string& devicePath) : devicePath_(devicePath) {}
    ndk::ScopedAStatus storeKey(const std::vector<int8_t>& kek) override;
    ndk::ScopedAStatus retrieveKey(std::vector<int8_t>* _aidl_return) override;

  private:
    const std::string devicePath_;
};

}  // namespace rebootescrow
}  // namespace hardware
}  // namespace android
}  // namespace aidl
