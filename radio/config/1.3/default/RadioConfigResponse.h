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

#ifndef ANDROID_HARDWARE_RADIO_CONFIG_V1_3_RADIOCONFIGRESPONSE_H
#define ANDROID_HARDWARE_RADIO_CONFIG_V1_3_RADIOCONFIGRESPONSE_H

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

using ::android::hardware::hidl_vec;
using ::android::hardware::Return;

using ::android::hardware::radio::V1_0::RadioResponseInfo;

struct RadioConfigResponse : public IRadioConfigResponse {
    // Methods from ::android::hardware::radio::config::V1_0::IRadioConfigResponse follow.
    Return<void> getSimSlotsStatusResponse(
            const RadioResponseInfo& info,
            const hidl_vec<V1_0::SimSlotStatus>& slotStatus) override;
    Return<void> setSimSlotsMappingResponse(const RadioResponseInfo& info) override;

    // Methods from ::android::hardware::radio::config::V1_1::IRadioConfigResponse follow.
    Return<void> getPhoneCapabilityResponse(const RadioResponseInfo& info,
                                            const V1_1::PhoneCapability& phoneCapability) override;
    Return<void> setPreferredDataModemResponse(const RadioResponseInfo& info) override;
    Return<void> setModemsConfigResponse(const RadioResponseInfo& info) override;
    Return<void> getModemsConfigResponse(const RadioResponseInfo& info,
                                         const V1_1::ModemsConfig& modemsConfig) override;

    // Methods from ::android::hardware::radio::config::V1_2::IRadioConfigResponse follow.
    Return<void> getSimSlotsStatusResponse_1_2(
            const RadioResponseInfo& info,
            const hidl_vec<V1_2::SimSlotStatus>& slotStatus) override;

    // Methods from ::android::hardware::radio::config::V1_3::IRadioConfigResponse follow.
    Return<void> getPhoneCapabilityResponse_1_3(
            const RadioResponseInfo& info, const V1_3::PhoneCapability& phoneCapability) override;
};

}  // namespace implementation
}  // namespace V1_3
}  // namespace config
}  // namespace radio
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_RADIO_CONFIG_V1_3_RADIOCONFIGRESPONSE_H
