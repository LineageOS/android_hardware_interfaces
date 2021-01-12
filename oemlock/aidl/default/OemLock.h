/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <aidl/android/hardware/oemlock/BnOemLock.h>

namespace aidl {
namespace android {
namespace hardware {
namespace oemlock {

using ::aidl::android::hardware::oemlock::IOemLock;
using ::aidl::android::hardware::oemlock::OemLockSecureStatus;

struct OemLock : public BnOemLock {
public:
    OemLock() = default;

    // Methods from ::android::hardware::oemlock::IOemLock follow.
    ::ndk::ScopedAStatus getName(std::string* out_name) override;
    ::ndk::ScopedAStatus isOemUnlockAllowedByCarrier(bool* out_allowed) override;
    ::ndk::ScopedAStatus isOemUnlockAllowedByDevice(bool* out_allowed) override;
    ::ndk::ScopedAStatus setOemUnlockAllowedByCarrier(bool in_allowed, const std::vector<uint8_t>& in_signature, OemLockSecureStatus* _aidl_return) override;
    ::ndk::ScopedAStatus setOemUnlockAllowedByDevice(bool in_allowed) override;
};

} // namespace oemlock
} // namespace hardware
} // namespace android
} // aidl
