/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <radio_config_hidl_hal_utils.h>

using ::android::hardware::radio::V1_0::RadioResponseInfo;

// SimSlotStatus slotStatus;

RadioConfigResponse::RadioConfigResponse(RadioConfigHidlTest& parent) : parent(parent) {}

Return<void> RadioConfigResponse::getSimSlotsStatusResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /* info */,
        const ::android::hardware::hidl_vec<
                ::android::hardware::radio::config::V1_0::SimSlotStatus>& /* slotStatus */) {
    return Void();
}

Return<void> RadioConfigResponse::getSimSlotsStatusResponse_1_2(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /* info */,
        const ::android::hardware::hidl_vec<SimSlotStatus>& /* slotStatus */) {
    return Void();
}

Return<void> RadioConfigResponse::setSimSlotsMappingResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /* info */) {
    return Void();
}

Return<void> RadioConfigResponse::getPhoneCapabilityResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& info,
        const PhoneCapability& phoneCapability) {
    rspInfo = info;
    phoneCap = phoneCapability;
    parent.notify(info.serial);
    return Void();
}

Return<void> RadioConfigResponse::setPreferredDataModemResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /* info */) {
    return Void();
}

Return<void> RadioConfigResponse::getModemsConfigResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /* info */,
        const ModemsConfig& /* mConfig */) {
    return Void();
}

Return<void> RadioConfigResponse::setModemsConfigResponse(
        const ::android::hardware::radio::V1_0::RadioResponseInfo& /* info */) {
    return Void();
}
