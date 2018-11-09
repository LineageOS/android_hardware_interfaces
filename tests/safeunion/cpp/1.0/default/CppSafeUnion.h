/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <android/hardware/tests/safeunion/cpp/1.0/ICppSafeUnion.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace tests {
namespace safeunion {
namespace cpp {
namespace V1_0 {
namespace implementation {

using ::android::sp;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;

struct CppSafeUnion : public ICppSafeUnion {
    Return<void> repeatPointerFmqSafeUnion(const ICppSafeUnion::PointerFmqSafeUnion& fmq,
                                           repeatPointerFmqSafeUnion_cb _hidl_cb) override;
    Return<void> repeatFmqSafeUnion(const ICppSafeUnion::FmqSafeUnion& fmq,
                                    repeatFmqSafeUnion_cb _hidl_cb) override;
};

extern "C" ICppSafeUnion* HIDL_FETCH_ICppSafeUnion(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace cpp
}  // namespace safeunion
}  // namespace tests
}  // namespace hardware
}  // namespace android
