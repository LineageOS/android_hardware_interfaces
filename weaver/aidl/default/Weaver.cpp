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

#include "Weaver.h"

namespace aidl {
namespace android {
namespace hardware {
namespace weaver {

// Methods from ::android::hardware::weaver::IWeaver follow.

::ndk::ScopedAStatus Weaver::getConfig(WeaverConfig* out_config) {
    (void)out_config;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Weaver::read(int32_t in_slotId, const std::vector<uint8_t>& in_key, WeaverReadResponse* out_response) {
    (void)in_slotId;
    (void)in_key;
    (void)out_response;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Weaver::write(int32_t in_slotId, const std::vector<uint8_t>& in_key, const std::vector<uint8_t>& in_value) {
    (void)in_slotId;
    (void)in_key;
    (void)in_value;
    return ::ndk::ScopedAStatus::ok();
}

} //namespace weaver
} //namespace hardware
} //namespace android
} //namespace aidl
