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

#include <radio_hidl_hal_utils_v1_2.h>

RadioConfigResponse::RadioConfigResponse(RadioHidlTest_v1_2& parent) : parent_v1_2(parent) {}

Return<void> RadioConfigResponse::getSimSlotsStatusResponse(
        const RadioResponseInfo& info,
        const ::android::hardware::hidl_vec<SimSlotStatus>& slotStatus) {
    rspInfo = info;
    simSlotStatus = slotStatus;
    parent_v1_2.notify(info.serial);
    return Void();
}

Return<void> RadioConfigResponse::setSimSlotsMappingResponse(const RadioResponseInfo& /* info */) {
    return Void();
}

Return<void> RadioConfigResponse::getPhoneCapabilityResponse(
        const RadioResponseInfo& info, const PhoneCapability& phoneCapability) {
    rspInfo = info;
    phoneCap = phoneCapability;
    parent_v1_2.notify(info.serial);
    return Void();
}

Return<void> RadioConfigResponse::setPreferredDataModemResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_2.notify(info.serial);
    return Void();
}

Return<void> RadioConfigResponse::getModemsConfigResponse(const RadioResponseInfo& info,
                                                          const ModemsConfig& /* mConfig */) {
    rspInfo = info;
    parent_v1_2.notify(info.serial);
    return Void();
}

Return<void> RadioConfigResponse::setModemsConfigResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_2.notify(info.serial);
    return Void();
}
