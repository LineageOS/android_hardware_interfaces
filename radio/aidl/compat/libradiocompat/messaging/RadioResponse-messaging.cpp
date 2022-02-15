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

#define RADIO_MODULE "MessagingResponse"

namespace android::hardware::radio::compat {

namespace aidl = ::aidl::android::hardware::radio::messaging;

void RadioResponse::setResponseFunction(std::shared_ptr<aidl::IRadioMessagingResponse> rmrCb) {
    mMessagingCb = rmrCb;
}

std::shared_ptr<aidl::IRadioMessagingResponse> RadioResponse::messagingCb() {
    return mMessagingCb.get();
}

Return<void> RadioResponse::acknowledgeIncomingGsmSmsWithPduResponse(
        const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    messagingCb()->acknowledgeIncomingGsmSmsWithPduResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::acknowledgeLastIncomingCdmaSmsResponse(
        const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    messagingCb()->acknowledgeLastIncomingCdmaSmsResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::acknowledgeLastIncomingGsmSmsResponse(
        const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    messagingCb()->acknowledgeLastIncomingGsmSmsResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::deleteSmsOnRuimResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    messagingCb()->deleteSmsOnRuimResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::deleteSmsOnSimResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    messagingCb()->deleteSmsOnSimResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::getCdmaBroadcastConfigResponse(
        const V1_0::RadioResponseInfo& info,
        const hidl_vec<V1_0::CdmaBroadcastSmsConfigInfo>& configs) {
    LOG_CALL << info.serial;
    messagingCb()->getCdmaBroadcastConfigResponse(toAidl(info), toAidl(configs));
    return {};
}

Return<void> RadioResponse::getGsmBroadcastConfigResponse(
        const V1_0::RadioResponseInfo& info, const hidl_vec<V1_0::GsmBroadcastSmsConfigInfo>& cfg) {
    LOG_CALL << info.serial;
    messagingCb()->getGsmBroadcastConfigResponse(toAidl(info), toAidl(cfg));
    return {};
}

Return<void> RadioResponse::getSmscAddressResponse(const V1_0::RadioResponseInfo& info,
                                                   const hidl_string& smsc) {
    LOG_CALL << info.serial;
    messagingCb()->getSmscAddressResponse(toAidl(info), smsc);
    return {};
}

Return<void> RadioResponse::reportSmsMemoryStatusResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    messagingCb()->reportSmsMemoryStatusResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::sendCdmaSmsExpectMoreResponse(const V1_0::RadioResponseInfo& info,
                                                          const V1_0::SendSmsResult& sms) {
    LOG_CALL << info.serial;
    messagingCb()->sendCdmaSmsExpectMoreResponse(toAidl(info), toAidl(sms));
    return {};
}

Return<void> RadioResponse::sendCdmaSmsExpectMoreResponse_1_6(const V1_6::RadioResponseInfo& info,
                                                              const V1_0::SendSmsResult& sms) {
    LOG_CALL << info.serial;
    messagingCb()->sendCdmaSmsExpectMoreResponse(toAidl(info), toAidl(sms));
    return {};
}

Return<void> RadioResponse::sendCdmaSmsResponse(const V1_0::RadioResponseInfo& info,
                                                const V1_0::SendSmsResult& sms) {
    LOG_CALL << info.serial;
    messagingCb()->sendCdmaSmsResponse(toAidl(info), toAidl(sms));
    return {};
}

Return<void> RadioResponse::sendCdmaSmsResponse_1_6(const V1_6::RadioResponseInfo& info,
                                                    const V1_0::SendSmsResult& sms) {
    LOG_CALL << info.serial;
    messagingCb()->sendCdmaSmsResponse(toAidl(info), toAidl(sms));
    return {};
}

Return<void> RadioResponse::sendImsSmsResponse(const V1_0::RadioResponseInfo& info,
                                               const V1_0::SendSmsResult& sms) {
    LOG_CALL << info.serial;
    messagingCb()->sendImsSmsResponse(toAidl(info), toAidl(sms));
    return {};
}

Return<void> RadioResponse::sendSMSExpectMoreResponse(const V1_0::RadioResponseInfo& info,
                                                      const V1_0::SendSmsResult& sms) {
    LOG_CALL << info.serial;
    messagingCb()->sendSmsExpectMoreResponse(toAidl(info), toAidl(sms));
    return {};
}

Return<void> RadioResponse::sendSmsExpectMoreResponse_1_6(const V1_6::RadioResponseInfo& info,
                                                          const V1_0::SendSmsResult& sms) {
    LOG_CALL << info.serial;
    messagingCb()->sendSmsExpectMoreResponse(toAidl(info), toAidl(sms));
    return {};
}

Return<void> RadioResponse::sendSmsResponse(const V1_0::RadioResponseInfo& info,
                                            const V1_0::SendSmsResult& sms) {
    LOG_CALL << info.serial;
    messagingCb()->sendSmsResponse(toAidl(info), toAidl(sms));
    return {};
}

Return<void> RadioResponse::sendSmsResponse_1_6(const V1_6::RadioResponseInfo& info,
                                                const V1_0::SendSmsResult& sms) {
    LOG_CALL << info.serial;
    messagingCb()->sendSmsResponse(toAidl(info), toAidl(sms));
    return {};
}

Return<void> RadioResponse::setCdmaBroadcastActivationResponse(
        const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    messagingCb()->setCdmaBroadcastActivationResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setCdmaBroadcastConfigResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    messagingCb()->setCdmaBroadcastConfigResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setGsmBroadcastActivationResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    messagingCb()->setGsmBroadcastActivationResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setGsmBroadcastConfigResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    messagingCb()->setGsmBroadcastConfigResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setSmscAddressResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    messagingCb()->setSmscAddressResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::writeSmsToRuimResponse(const V1_0::RadioResponseInfo& info,
                                                   uint32_t index) {
    LOG_CALL << info.serial << ' ' << index;
    messagingCb()->writeSmsToRuimResponse(toAidl(info), index);
    return {};
}

Return<void> RadioResponse::writeSmsToSimResponse(const V1_0::RadioResponseInfo& info,
                                                  int32_t index) {
    LOG_CALL << info.serial << ' ' << index;
    messagingCb()->writeSmsToSimResponse(toAidl(info), index);
    return {};
}

}  // namespace android::hardware::radio::compat
