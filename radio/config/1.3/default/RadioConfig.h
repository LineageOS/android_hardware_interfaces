/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.1
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_HARDWARE_RADIO_CONFIG_V1_3_RADIOCONFIG_H
#define ANDROID_HARDWARE_RADIO_CONFIG_V1_3_RADIOCONFIG_H

#include <android/hardware/radio/config/1.3/IRadioConfig.h>
#include <android/hardware/radio/config/1.3/IRadioConfigIndication.h>
#include <android/hardware/radio/config/1.3/IRadioConfigResponse.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace radio {
namespace config {
namespace V1_3 {
namespace implementation {

using namespace ::android::hardware::radio::config;

using ::android::sp;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;

struct RadioConfig : public V1_3::IRadioConfig {
    sp<V1_0::IRadioConfigResponse> mRadioConfigResponse;
    sp<V1_0::IRadioConfigIndication> mRadioConfigIndication;
    sp<V1_1::IRadioConfigResponse> mRadioConfigResponseV1_1;
    sp<V1_1::IRadioConfigIndication> mRadioConfigIndicationV1_1;
    sp<V1_2::IRadioConfigResponse> mRadioConfigResponseV1_2;
    sp<V1_2::IRadioConfigIndication> mRadioConfigIndicationV1_2;
    sp<V1_3::IRadioConfigResponse> mRadioConfigResponseV1_3;
    sp<V1_3::IRadioConfigIndication> mRadioConfigIndicationV1_3;

    // Methods from ::android::hardware::radio::config::V1_0::IRadioConfig follow.
    Return<void> setResponseFunctions(
            const sp<V1_0::IRadioConfigResponse>& radioConfigResponse,
            const sp<V1_0::IRadioConfigIndication>& radioConfigIndication);
    Return<void> getSimSlotsStatus(int32_t serial);
    Return<void> setSimSlotsMapping(int32_t serial, const hidl_vec<uint32_t>& slotMap);

    // Methods from ::android::hardware::radio::config::V1_1::IRadioConfig follow.
    Return<void> getPhoneCapability(int32_t serial);
    Return<void> setPreferredDataModem(int32_t serial, uint8_t modemId);
    Return<void> setModemsConfig(int32_t serial, const V1_1::ModemsConfig& modemsConfig);
    Return<void> getModemsConfig(int32_t serial);

    // Methods from ::android::hardware::radio::config::V1_3::IRadioConfig follow.
    Return<void> getPhoneCapability_1_3(int32_t serial);
};

}  // namespace implementation
}  // namespace V1_3
}  // namespace config
}  // namespace radio
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_RADIO_CONFIG_V1_3_RADIOCONFIG_H
