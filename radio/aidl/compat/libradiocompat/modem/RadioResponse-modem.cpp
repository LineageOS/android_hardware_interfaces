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

#include <libradiocompat/RadioResponse.h>

#include "commonStructs.h"
#include "debug.h"
#include "structs.h"

#include "collections.h"

#define RADIO_MODULE "ModemResponse"

namespace android::hardware::radio::compat {

namespace aidl = ::aidl::android::hardware::radio::modem;

void RadioResponse::setResponseFunction(std::shared_ptr<aidl::IRadioModemResponse> modemCb) {
    mModemCb = modemCb;
}

std::shared_ptr<aidl::IRadioModemResponse> RadioResponse::modemCb() {
    return mModemCb.get();
}

Return<void> RadioResponse::enableModemResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    modemCb()->enableModemResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::getBasebandVersionResponse(const V1_0::RadioResponseInfo& info,
                                                       const hidl_string& version) {
    LOG_CALL << info.serial;
    modemCb()->getBasebandVersionResponse(toAidl(info), version);
    return {};
}

Return<void> RadioResponse::getDeviceIdentityResponse(  //
        const V1_0::RadioResponseInfo& info, const hidl_string& imei, const hidl_string& imeisv,
        const hidl_string& esn, const hidl_string& meid) {
    LOG_CALL << info.serial;
    modemCb()->getDeviceIdentityResponse(toAidl(info), imei, imeisv, esn, meid);
    return {};
}

Return<void> RadioResponse::getHardwareConfigResponse(
        const V1_0::RadioResponseInfo& info, const hidl_vec<V1_0::HardwareConfig>& config) {
    LOG_CALL << info.serial;
    modemCb()->getHardwareConfigResponse(toAidl(info), toAidl(config));
    return {};
}

Return<void> RadioResponse::getModemActivityInfoResponse(
        const V1_0::RadioResponseInfo& info, const V1_0::ActivityStatsInfo& activityInfo) {
    LOG_CALL << info.serial;
    modemCb()->getModemActivityInfoResponse(toAidl(info), toAidl(activityInfo));
    return {};
}

Return<void> RadioResponse::getModemStackStatusResponse(const V1_0::RadioResponseInfo& info,
                                                        bool isEnabled) {
    LOG_CALL << info.serial;
    modemCb()->getModemStackStatusResponse(toAidl(info), isEnabled);
    return {};
}

Return<void> RadioResponse::getRadioCapabilityResponse(const V1_0::RadioResponseInfo& info,
                                                       const V1_0::RadioCapability& rc) {
    LOG_CALL << info.serial;
    modemCb()->getRadioCapabilityResponse(toAidl(info), toAidl(rc));
    return {};
}

Return<void> RadioResponse::nvReadItemResponse(const V1_0::RadioResponseInfo& info,
                                               const hidl_string& result) {
    LOG_CALL << info.serial;
    modemCb()->nvReadItemResponse(toAidl(info), result);
    return {};
}

Return<void> RadioResponse::nvResetConfigResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    modemCb()->nvResetConfigResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::nvWriteCdmaPrlResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    modemCb()->nvWriteCdmaPrlResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::nvWriteItemResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    modemCb()->nvWriteItemResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::requestShutdownResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    modemCb()->requestShutdownResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::sendDeviceStateResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    modemCb()->sendDeviceStateResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setRadioCapabilityResponse(const V1_0::RadioResponseInfo& info,
                                                       const V1_0::RadioCapability& rc) {
    LOG_CALL << info.serial;
    modemCb()->setRadioCapabilityResponse(toAidl(info), toAidl(rc));
    return {};
}

Return<void> RadioResponse::setRadioPowerResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    modemCb()->setRadioPowerResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setRadioPowerResponse_1_5(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    modemCb()->setRadioPowerResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setRadioPowerResponse_1_6(const V1_6::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    modemCb()->setRadioPowerResponse(toAidl(info));
    return {};
}

}  // namespace android::hardware::radio::compat
