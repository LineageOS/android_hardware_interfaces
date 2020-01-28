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

#include "RadioConfigResponse.h"

namespace android {
namespace hardware {
namespace radio {
namespace config {
namespace V1_3 {
namespace implementation {

// Methods from ::android::hardware::radio::config::V1_0::IRadioConfigResponse follow.
Return<void> RadioConfigResponse::getSimSlotsStatusResponse(
        const RadioResponseInfo& /* info */,
        const hidl_vec<V1_0::SimSlotStatus>& /* slotStatus */) {
    // TODO implement
    return Void();
}

Return<void> RadioConfigResponse::setSimSlotsMappingResponse(const RadioResponseInfo& /* info */) {
    // TODO implement
    return Void();
}

// Methods from ::android::hardware::radio::config::V1_1::IRadioConfigResponse follow.
Return<void> RadioConfigResponse::getPhoneCapabilityResponse(
        const RadioResponseInfo& /* info */, const V1_1::PhoneCapability& /* phoneCapability */) {
    // TODO implement
    return Void();
}

Return<void> RadioConfigResponse::setPreferredDataModemResponse(
        const RadioResponseInfo& /* info */) {
    // TODO implement
    return Void();
}

Return<void> RadioConfigResponse::setModemsConfigResponse(const RadioResponseInfo& /* info */) {
    // TODO implement
    return Void();
}

Return<void> RadioConfigResponse::getModemsConfigResponse(
        const RadioResponseInfo& /* info */, const V1_1::ModemsConfig& /* modemsConfig */) {
    // TODO implement
    return Void();
}

// Methods from ::android::hardware::radio::config::V1_2::IRadioConfigResponse follow.
Return<void> RadioConfigResponse::getSimSlotsStatusResponse_1_2(
        const RadioResponseInfo& /* info */,
        const hidl_vec<V1_2::SimSlotStatus>& /* slotStatus */) {
    // TODO implement
    return Void();
}

// Methods from ::android::hardware::radio::config::V1_3::IRadioConfigResponse follow.
Return<void> RadioConfigResponse::getPhoneCapabilityResponse_1_3(
        const RadioResponseInfo& /* info */, const V1_3::PhoneCapability& /* phoneCapability */) {
    // TODO implement
    return Void();
}

}  // namespace implementation
}  // namespace V1_3
}  // namespace config
}  // namespace radio
}  // namespace hardware
}  // namespace android
