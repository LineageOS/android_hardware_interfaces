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

#include "OemLock.h"

namespace aidl {
namespace android {
namespace hardware {
namespace oemlock {

// Methods from ::android::hardware::oemlock::IOemLock follow.

::ndk::ScopedAStatus OemLock::getName(std::string *out_name) {
    (void)out_name;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus OemLock::setOemUnlockAllowedByCarrier(bool in_allowed, const std::vector<uint8_t> &in_signature, OemLockSecureStatus *_aidl_return) {
    (void)in_allowed;
    (void)in_signature;
    (void)_aidl_return;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus OemLock::isOemUnlockAllowedByCarrier(bool *out_allowed) {
    (void)out_allowed;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus OemLock::setOemUnlockAllowedByDevice(bool in_allowed) {
    (void)in_allowed;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus OemLock::isOemUnlockAllowedByDevice(bool *out_allowed) {
    (void)out_allowed;
    return ::ndk::ScopedAStatus::ok();
}

} // namespace oemlock
} // namespace hardware
} // namespace android
} // aidl
