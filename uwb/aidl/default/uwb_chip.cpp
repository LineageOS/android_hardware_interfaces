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

#include "uwb.h"

namespace android {
namespace hardware {
namespace uwb {
namespace impl {
using namespace ::aidl::android::hardware::uwb;

UwbChip::UwbChip(const std::string& name) : name_(name) {}
UwbChip::~UwbChip() {}

::ndk::ScopedAStatus UwbChip::getName(std::string* name) {
    *name = name_;
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus UwbChip::open(
        const std::shared_ptr<IUwbClientCallback>& /* clientCallback */) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

::ndk::ScopedAStatus UwbChip::close() {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

::ndk::ScopedAStatus UwbChip::coreInit() {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

::ndk::ScopedAStatus UwbChip::getSupportedVendorUciVersion(int32_t* /* version */) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

::ndk::ScopedAStatus UwbChip::sendUciMessage(const std::vector<uint8_t>& /* data */,
                                             int32_t* /* bytes_written */) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}
}  // namespace impl
}  // namespace uwb
}  // namespace hardware
}  // namespace android
