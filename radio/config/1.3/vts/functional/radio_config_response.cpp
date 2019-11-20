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

using namespace ::android::hardware::radio::config;

using ::android::hardware::hidl_vec;

using ::android::hardware::radio::V1_0::RadioResponseInfo;

RadioConfigResponse::RadioConfigResponse(RadioConfigHidlTest& parent) : parent(parent) {}

/* 1.0 Apis */
Return<void> RadioConfigResponse::getSimSlotsStatusResponse(
        const RadioResponseInfo& /* info */,
        const hidl_vec<V1_0::SimSlotStatus>& /* slotStatus */) {
    return Void();
}

Return<void> RadioConfigResponse::setSimSlotsMappingResponse(const RadioResponseInfo& /* info */) {
    return Void();
}

/* 1.1 Apis */
Return<void> RadioConfigResponse::getPhoneCapabilityResponse(
        const RadioResponseInfo& info, const V1_1::PhoneCapability& phoneCapability) {
    rspInfo = info;
    phoneCap_1_1 = phoneCapability;
    parent.notify(info.serial);
    return Void();
}

Return<void> RadioConfigResponse::setPreferredDataModemResponse(
        const RadioResponseInfo& /* info */) {
    return Void();
}

Return<void> RadioConfigResponse::getModemsConfigResponse(const RadioResponseInfo& /* info */,
                                                          const V1_1::ModemsConfig& /* mConfig */) {
    return Void();
}

Return<void> RadioConfigResponse::setModemsConfigResponse(const RadioResponseInfo& /* info */) {
    return Void();
}

/* 1.2 Apis */
Return<void> RadioConfigResponse::getSimSlotsStatusResponse_1_2(
        const RadioResponseInfo& /* info */,
        const hidl_vec<V1_2::SimSlotStatus>& /* slotStatus */) {
    return Void();
}

/* 1.3 Apis */
Return<void> RadioConfigResponse::getPhoneCapabilityResponse_1_3(
        const RadioResponseInfo& info, const V1_3::PhoneCapability& phoneCapability) {
    rspInfo = info;
    phoneCap_1_3 = phoneCapability;
    parent.notify(info.serial);
    return Void();
}
