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

#include <aidl/android/hardware/weaver/BnWeaver.h>

namespace aidl {
namespace android {
namespace hardware {
namespace weaver {

using ::aidl::android::hardware::weaver::WeaverConfig;
using ::aidl::android::hardware::weaver::WeaverReadResponse;

struct Weaver : public BnWeaver {
public:
    Weaver() = default;

    // Methods from ::android::hardware::weaver::IWeaver follow.
    ::ndk::ScopedAStatus getConfig(WeaverConfig* _aidl_return) override;
    ::ndk::ScopedAStatus read(int32_t in_slotId, const std::vector<uint8_t>& in_key, WeaverReadResponse* _aidl_return) override;
    ::ndk::ScopedAStatus write(int32_t in_slotId, const std::vector<uint8_t>& in_key, const std::vector<uint8_t>& in_value) override;
};

}  // namespace weaver
}  // namespace hardware
}  // namespace android
}  // namespace aidl
