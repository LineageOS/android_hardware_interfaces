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

#define RADIO_MODULE "SimResponse"

namespace android::hardware::radio::compat {

namespace aidl = ::aidl::android::hardware::radio::sim;

void RadioResponse::setResponseFunction(std::shared_ptr<aidl::IRadioSimResponse> simCb) {
    CHECK(simCb);
    mSimCb = simCb;
}

Return<void> RadioResponse::areUiccApplicationsEnabledResponse(const V1_0::RadioResponseInfo& info,
                                                               bool enabled) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->areUiccApplicationsEnabledResponse(toAidl(info), enabled);
    return {};
}

Return<void> RadioResponse::changeIccPin2ForAppResponse(const V1_0::RadioResponseInfo& info,
                                                        int32_t remainingRetries) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->changeIccPin2ForAppResponse(toAidl(info), remainingRetries);
    return {};
}

Return<void> RadioResponse::changeIccPinForAppResponse(const V1_0::RadioResponseInfo& info,
                                                       int32_t remainingRetries) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->changeIccPinForAppResponse(toAidl(info), remainingRetries);
    return {};
}

Return<void> RadioResponse::enableUiccApplicationsResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->enableUiccApplicationsResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::getAllowedCarriersResponse(  //
        const V1_0::RadioResponseInfo& info, bool allAllowed, const V1_0::CarrierRestrictions& cr) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    aidl::CarrierRestrictions aidlCr = toAidl(cr);
    if (allAllowed) aidlCr = {};
    mSimCb->getAllowedCarriersResponse(toAidl(info), aidlCr, {});
    return {};
}

Return<void> RadioResponse::getAllowedCarriersResponse_1_4(
        const V1_0::RadioResponseInfo& info, const V1_4::CarrierRestrictionsWithPriority& carriers,
        V1_4::SimLockMultiSimPolicy multiSimPolicy) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->getAllowedCarriersResponse(toAidl(info), toAidl(carriers),
                                       aidl::SimLockMultiSimPolicy(multiSimPolicy));
    return {};
}

Return<void> RadioResponse::getCDMASubscriptionResponse(
        const V1_0::RadioResponseInfo& info, const hidl_string& mdn, const hidl_string& hSid,
        const hidl_string& hNid, const hidl_string& min, const hidl_string& prl) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->getCdmaSubscriptionResponse(toAidl(info), mdn, hSid, hNid, min, prl);
    return {};
}

Return<void> RadioResponse::getCdmaSubscriptionSourceResponse(const V1_0::RadioResponseInfo& info,
                                                              V1_0::CdmaSubscriptionSource s) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->getCdmaSubscriptionSourceResponse(toAidl(info), aidl::CdmaSubscriptionSource(s));
    return {};
}

Return<void> RadioResponse::getFacilityLockForAppResponse(const V1_0::RadioResponseInfo& info,
                                                          int32_t response) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->getFacilityLockForAppResponse(toAidl(info), response);
    return {};
}

Return<void> RadioResponse::getIccCardStatusResponse(const V1_0::RadioResponseInfo& info,
                                                     const V1_0::CardStatus& cardStatus) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->getIccCardStatusResponse(toAidl(info), toAidl(cardStatus));
    return {};
}

Return<void> RadioResponse::getIccCardStatusResponse_1_2(const V1_0::RadioResponseInfo& info,
                                                         const V1_2::CardStatus& cardStatus) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->getIccCardStatusResponse(toAidl(info), toAidl(cardStatus));
    return {};
}

Return<void> RadioResponse::getIccCardStatusResponse_1_4(const V1_0::RadioResponseInfo& info,
                                                         const V1_4::CardStatus& cardStatus) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->getIccCardStatusResponse(toAidl(info), toAidl(cardStatus));
    return {};
}

Return<void> RadioResponse::getIccCardStatusResponse_1_5(const V1_0::RadioResponseInfo& info,
                                                         const V1_5::CardStatus& cardStatus) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->getIccCardStatusResponse(toAidl(info), toAidl(cardStatus));
    return {};
}

Return<void> RadioResponse::getIMSIForAppResponse(const V1_0::RadioResponseInfo& info,
                                                  const hidl_string& imsi) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->getImsiForAppResponse(toAidl(info), imsi);
    return {};
}

Return<void> RadioResponse::getSimPhonebookCapacityResponse(
        const V1_6::RadioResponseInfo& info, const V1_6::PhonebookCapacity& capacity) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->getSimPhonebookCapacityResponse(toAidl(info), toAidl(capacity));
    return {};
}

Return<void> RadioResponse::getSimPhonebookRecordsResponse(const V1_6::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->getSimPhonebookRecordsResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::iccCloseLogicalChannelResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->iccCloseLogicalChannelResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::iccIOForAppResponse(const V1_0::RadioResponseInfo& info,
                                                const V1_0::IccIoResult& iccIo) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->iccIoForAppResponse(toAidl(info), toAidl(iccIo));
    return {};
}

Return<void> RadioResponse::iccOpenLogicalChannelResponse(  //
        const V1_0::RadioResponseInfo& info, int32_t chanId, const hidl_vec<int8_t>& selectResp) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->iccOpenLogicalChannelResponse(toAidl(info), chanId, toAidl(selectResp));
    return {};
}

Return<void> RadioResponse::iccTransmitApduBasicChannelResponse(const V1_0::RadioResponseInfo& info,
                                                                const V1_0::IccIoResult& result) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->iccTransmitApduBasicChannelResponse(toAidl(info), toAidl(result));
    return {};
}

Return<void> RadioResponse::iccTransmitApduLogicalChannelResponse(
        const V1_0::RadioResponseInfo& info, const V1_0::IccIoResult& result) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->iccTransmitApduLogicalChannelResponse(toAidl(info), toAidl(result));
    return {};
}

Return<void> RadioResponse::reportStkServiceIsRunningResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->reportStkServiceIsRunningResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::requestIccSimAuthenticationResponse(const V1_0::RadioResponseInfo& info,
                                                                const V1_0::IccIoResult& result) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->requestIccSimAuthenticationResponse(toAidl(info), toAidl(result));
    return {};
}

Return<void> RadioResponse::requestIsimAuthenticationResponse(const V1_0::RadioResponseInfo& info,
                                                              const hidl_string&) {
    LOG_CALL << info.serial;
    LOG(ERROR) << "requestIsimAuthenticationResponse is not supposed to be called";
    return {};
}

Return<void> RadioResponse::sendEnvelopeResponse(const V1_0::RadioResponseInfo& info,
                                                 const hidl_string& commandResponse) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->sendEnvelopeResponse(toAidl(info), commandResponse);
    return {};
}

Return<void> RadioResponse::sendEnvelopeWithStatusResponse(const V1_0::RadioResponseInfo& info,
                                                           const V1_0::IccIoResult& iccIo) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->sendEnvelopeWithStatusResponse(toAidl(info), toAidl(iccIo));
    return {};
}

Return<void> RadioResponse::sendTerminalResponseToSimResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->sendTerminalResponseToSimResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setAllowedCarriersResponse(const V1_0::RadioResponseInfo& info,
                                                       int32_t numAllowed) {
    LOG_CALL << info.serial << ' ' << numAllowed;
    CHECK_CB(mSimCb);
    mSimCb->setAllowedCarriersResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setAllowedCarriersResponse_1_4(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->setAllowedCarriersResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setCarrierInfoForImsiEncryptionResponse(
        const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->setCarrierInfoForImsiEncryptionResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setCdmaSubscriptionSourceResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->setCdmaSubscriptionSourceResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setFacilityLockForAppResponse(const V1_0::RadioResponseInfo& info,
                                                          int32_t retry) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->setFacilityLockForAppResponse(toAidl(info), retry);
    return {};
}

Return<void> RadioResponse::setSimCardPowerResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->setSimCardPowerResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setSimCardPowerResponse_1_1(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->setSimCardPowerResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setSimCardPowerResponse_1_6(const V1_6::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->setSimCardPowerResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::setUiccSubscriptionResponse(const V1_0::RadioResponseInfo& info) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->setUiccSubscriptionResponse(toAidl(info));
    return {};
}

Return<void> RadioResponse::supplyIccPin2ForAppResponse(const V1_0::RadioResponseInfo& info,
                                                        int32_t remainingRetries) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->supplyIccPin2ForAppResponse(toAidl(info), remainingRetries);
    return {};
}

Return<void> RadioResponse::supplyIccPinForAppResponse(const V1_0::RadioResponseInfo& info,
                                                       int32_t remainingRetries) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->supplyIccPinForAppResponse(toAidl(info), remainingRetries);
    return {};
}

Return<void> RadioResponse::supplyIccPuk2ForAppResponse(const V1_0::RadioResponseInfo& info,
                                                        int32_t remainingRetries) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->supplyIccPuk2ForAppResponse(toAidl(info), remainingRetries);
    return {};
}

Return<void> RadioResponse::supplyIccPukForAppResponse(const V1_0::RadioResponseInfo& info,
                                                       int32_t remainingRetries) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->supplyIccPukForAppResponse(toAidl(info), remainingRetries);
    return {};
}

Return<void> RadioResponse::supplySimDepersonalizationResponse(const V1_0::RadioResponseInfo& info,
                                                               V1_5::PersoSubstate persoType,
                                                               int32_t rRet) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->supplySimDepersonalizationResponse(toAidl(info), aidl::PersoSubstate(persoType), rRet);
    return {};
}

Return<void> RadioResponse::updateSimPhonebookRecordsResponse(const V1_6::RadioResponseInfo& info,
                                                              int32_t updatedRecordIndex) {
    LOG_CALL << info.serial;
    CHECK_CB(mSimCb);
    mSimCb->updateSimPhonebookRecordsResponse(toAidl(info), updatedRecordIndex);
    return {};
}

}  // namespace android::hardware::radio::compat
