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

#include "Memtrack.h"

namespace aidl {
namespace android {
namespace hardware {
namespace memtrack {

namespace {

// LINT.IfChange
constexpr char kMemtrackDefaultMsg[] = "memtrack default implementation";
// LINT.ThenChange(/frameworks/native/services/memtrackproxy/MemtrackProxy.cpp)

}  // namespace

ndk::ScopedAStatus Memtrack::getMemory([[maybe_unused]] int pid, [[maybe_unused]] MemtrackType type,
                                       [[maybe_unused]] std::vector<MemtrackRecord>* _aidl_return) {
    return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_UNSUPPORTED_OPERATION,
                                                            kMemtrackDefaultMsg);
}

ndk::ScopedAStatus Memtrack::getGpuDeviceInfo(std::vector<DeviceInfo>* _aidl_return) {
    _aidl_return->clear();
    DeviceInfo dev_info = {.id = 0, .name = "virtio_gpu"};
    _aidl_return->emplace_back(dev_info);
    return ndk::ScopedAStatus::ok();
}

}  // namespace memtrack
}  // namespace hardware
}  // namespace android
}  // namespace aidl
