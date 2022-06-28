/*
 * Copyright 2021, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <android-base/logging.h>

#include "uwb.h"

namespace {
static constexpr char kDefaultChipName[] = "default";

}  // namespace

namespace android {
namespace hardware {
namespace uwb {
namespace impl {
using namespace ::aidl::android::hardware::uwb;

// The default implementation of the HAL assumes 1 chip on the device.
Uwb::Uwb() : chips_({{kDefaultChipName, ndk::SharedRefBase::make<UwbChip>(kDefaultChipName)}}) {}

Uwb::~Uwb() {}

::ndk::ScopedAStatus Uwb::getChips(std::vector<std::string>* names) {
    for (const auto& chip : chips_) {
        names->push_back(chip.first);
    }
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Uwb::getChip(const std::string& name, std::shared_ptr<IUwbChip>* chip) {
    const auto chip_found = chips_.find(name);
    if (chip_found == chips_.end()) {
        LOG(ERROR) << "Unknown chip name" << name;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    *chip = chip_found->second;
    return ndk::ScopedAStatus::ok();
}
}  // namespace impl
}  // namespace uwb
}  // namespace hardware
}  // namespace android
