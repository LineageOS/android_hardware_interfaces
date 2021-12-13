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

#include <aidl/android/hardware/radio/modem/ActivityStatsInfo.h>
#include <aidl/android/hardware/radio/modem/ActivityStatsTechSpecificInfo.h>
#include <aidl/android/hardware/radio/modem/HardwareConfig.h>
#include <aidl/android/hardware/radio/modem/HardwareConfigModem.h>
#include <aidl/android/hardware/radio/modem/HardwareConfigSim.h>
#include <aidl/android/hardware/radio/modem/NvWriteItem.h>
#include <aidl/android/hardware/radio/modem/RadioCapability.h>
#include <android/hardware/radio/1.0/types.h>

namespace android::hardware::radio::compat {

V1_0::NvWriteItem toHidl(const ::aidl::android::hardware::radio::modem::NvWriteItem& item);

::aidl::android::hardware::radio::modem::RadioCapability toAidl(const V1_0::RadioCapability& capa);
V1_0::RadioCapability toHidl(const ::aidl::android::hardware::radio::modem::RadioCapability& capa);

::aidl::android::hardware::radio::modem::HardwareConfig toAidl(const V1_0::HardwareConfig& config);

::aidl::android::hardware::radio::modem::HardwareConfigModem  //
toAidl(const V1_0::HardwareConfigModem& modem);

::aidl::android::hardware::radio::modem::HardwareConfigSim toAidl(const V1_0::HardwareConfigSim& s);

::aidl::android::hardware::radio::modem::ActivityStatsInfo toAidl(const V1_0::ActivityStatsInfo& i);

}  // namespace android::hardware::radio::compat
