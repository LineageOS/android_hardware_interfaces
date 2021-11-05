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

#include "debug.h"

// TODO(b/203699028): remove when fully implemented
#pragma GCC diagnostic ignored "-Wunused-parameter"

#define RADIO_MODULE "Common"

namespace android::hardware::radio::compat {

Return<void> RadioResponse::acknowledgeRequest(int32_t serial) {
    LOG_CALL << serial;
    // TODO(b/203699028): send to correct requestor or confirm if spam is not a problem
    if (mDataCb) mDataCb->acknowledgeRequest(serial);
    if (mMessagingCb) mMessagingCb->acknowledgeRequest(serial);
    if (mNetworkCb) mNetworkCb->acknowledgeRequest(serial);
    if (mSimCb) mSimCb->acknowledgeRequest(serial);
    if (mVoiceCb) mVoiceCb->acknowledgeRequest(serial);
    return {};
}

Return<void> RadioResponse::setRadioPowerResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::getBasebandVersionResponse(const V1_0::RadioResponseInfo& info,
                                                       const hidl_string& version) {
    return {};
}

Return<void> RadioResponse::getDeviceIdentityResponse(  //
        const V1_0::RadioResponseInfo& info, const hidl_string& imei, const hidl_string& imeisv,
        const hidl_string& esn, const hidl_string& meid) {
    return {};
}

Return<void> RadioResponse::nvReadItemResponse(const V1_0::RadioResponseInfo& info,
                                               const hidl_string& result) {
    return {};
}

Return<void> RadioResponse::nvWriteItemResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::nvWriteCdmaPrlResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::nvResetConfigResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::getHardwareConfigResponse(
        const V1_0::RadioResponseInfo& info, const hidl_vec<V1_0::HardwareConfig>& config) {
    return {};
}

Return<void> RadioResponse::requestShutdownResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::getRadioCapabilityResponse(const V1_0::RadioResponseInfo& info,
                                                       const V1_0::RadioCapability& rc) {
    return {};
}

Return<void> RadioResponse::setRadioCapabilityResponse(const V1_0::RadioResponseInfo& info,
                                                       const V1_0::RadioCapability& rc) {
    return {};
}

Return<void> RadioResponse::getModemActivityInfoResponse(
        const V1_0::RadioResponseInfo& info, const V1_0::ActivityStatsInfo& activityInfo) {
    return {};
}

Return<void> RadioResponse::sendDeviceStateResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::enableModemResponse(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::getModemStackStatusResponse(const V1_0::RadioResponseInfo& info,
                                                        bool isEnabled) {
    return {};
}

Return<void> RadioResponse::setRadioPowerResponse_1_5(const V1_0::RadioResponseInfo& info) {
    return {};
}

Return<void> RadioResponse::setRadioPowerResponse_1_6(const V1_6::RadioResponseInfo& info) {
    return {};
}

}  // namespace android::hardware::radio::compat
