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

#include <aidl/android/hardware/memtrack/BnMemtrack.h>
#include <aidl/android/hardware/memtrack/DeviceInfo.h>
#include <aidl/android/hardware/memtrack/MemtrackRecord.h>
#include <aidl/android/hardware/memtrack/MemtrackType.h>

namespace aidl {
namespace android {
namespace hardware {
namespace memtrack {

class Memtrack : public BnMemtrack {
    ndk::ScopedAStatus getMemory(int pid, MemtrackType type,
                                 std::vector<MemtrackRecord>* _aidl_return) override;

    ndk::ScopedAStatus getGpuDeviceInfo(std::vector<DeviceInfo>* _aidl_return) override;
};

}  // namespace memtrack
}  // namespace hardware
}  // namespace android
}  // namespace aidl
