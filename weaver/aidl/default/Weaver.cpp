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
#include <array>

namespace aidl {
namespace android {
namespace hardware {
namespace weaver {

struct Slotinfo {
    int slot_id;
    std::vector<uint8_t> key;
    std::vector<uint8_t> value;
};

std::array<struct Slotinfo, 16> slot_array;
// Methods from ::android::hardware::weaver::IWeaver follow.

::ndk::ScopedAStatus Weaver::getConfig(WeaverConfig* out_config) {
    *out_config = {16, 16, 16};
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Weaver::read(int32_t in_slotId, const std::vector<uint8_t>& in_key, WeaverReadResponse* out_response) {
    using ::aidl::android::hardware::weaver::WeaverReadStatus;

    if (in_slotId > 15 || in_key.size() > 16) {
        *out_response = {0, {}, WeaverReadStatus::FAILED};
        return ndk::ScopedAStatus::ok();
    }

    if (slot_array[in_slotId].key != in_key) {
        *out_response = {0, {}, WeaverReadStatus::INCORRECT_KEY};
        return ndk::ScopedAStatus::ok();
    }

    *out_response = {0, slot_array[in_slotId].value, WeaverReadStatus::OK};

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Weaver::write(int32_t in_slotId, const std::vector<uint8_t>& in_key, const std::vector<uint8_t>& in_value) {

    if (in_slotId > 15 || in_key.size() > 16 || in_value.size() > 16)
        return ::ndk::ScopedAStatus::fromStatus(STATUS_FAILED_TRANSACTION);

    slot_array[in_slotId].key = in_key;
    slot_array[in_slotId].value = in_value;

    return ::ndk::ScopedAStatus::ok();
}

} //namespace weaver
} //namespace hardware
} //namespace android
} //namespace aidl
