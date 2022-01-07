/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include <aidl/android/hardware/radio/config/PhoneCapability.h>
#include <aidl/android/hardware/radio/config/SimSlotStatus.h>
#include <aidl/android/hardware/radio/config/SlotPortMapping.h>
#include <android/hardware/radio/config/1.1/types.h>
#include <android/hardware/radio/config/1.2/types.h>

namespace android::hardware::radio::compat {

uint32_t toHidl(const aidl::android::hardware::radio::config::SlotPortMapping& slotPortMapping);

aidl::android::hardware::radio::config::SimSlotStatus  //
toAidl(const config::V1_0::SimSlotStatus& sst);
aidl::android::hardware::radio::config::SimSlotStatus  //
toAidl(const config::V1_2::SimSlotStatus& sst);

uint8_t toAidl(const config::V1_1::ModemInfo& info);

aidl::android::hardware::radio::config::PhoneCapability  //
toAidl(const config::V1_1::PhoneCapability& pc);

}  // namespace android::hardware::radio::compat
