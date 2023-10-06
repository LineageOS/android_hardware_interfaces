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

#include <libradiocompat/RadioSim.h>

#include "commonStructs.h"
#include "debug.h"
#include "structs.h"

#include "collections.h"

#define RADIO_MODULE "Sim"

namespace android::hardware::radio::compat {

using ::ndk::ScopedAStatus;
namespace aidl = ::aidl::android::hardware::radio::sim;
constexpr auto ok = &ScopedAStatus::ok;

std::shared_ptr<aidl::IRadioSimResponse> RadioSim::respond() {
    return mCallbackManager->response().simCb();
}

ScopedAStatus RadioSim::areUiccApplicationsEnabled(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->areUiccApplicationsEnabled(serial);
    return ok();
}

ScopedAStatus RadioSim::changeIccPin2ForApp(int32_t serial, const std::string& oldPin2,
                                            const std::string& newPin2, const std::string& aid) {
    LOG_CALL << serial;
    mHal1_5->changeIccPin2ForApp(serial, oldPin2, newPin2, aid);
    return ok();
}

ScopedAStatus RadioSim::changeIccPinForApp(int32_t serial, const std::string& oldPin,
                                           const std::string& newPin, const std::string& aid) {
    LOG_CALL << serial;
    mHal1_5->changeIccPinForApp(serial, oldPin, newPin, aid);
    return ok();
}

ScopedAStatus RadioSim::enableUiccApplications(int32_t serial, bool enable) {
    LOG_CALL << serial;
    mHal1_5->enableUiccApplications(serial, enable);
    return ok();
}

ScopedAStatus RadioSim::getAllowedCarriers(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->getAllowedCarriers_1_4(serial);
    return ok();
}

ScopedAStatus RadioSim::getCdmaSubscription(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->getCDMASubscription(serial);
    return ok();
}

ScopedAStatus RadioSim::getCdmaSubscriptionSource(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->getCdmaSubscriptionSource(serial);
    return ok();
}

ScopedAStatus RadioSim::getFacilityLockForApp(  //
        int32_t serial, const std::string& facility, const std::string& password,
        int32_t serviceClass, const std::string& appId) {
    LOG_CALL << serial;
    mHal1_5->getFacilityLockForApp(serial, facility, password, serviceClass, appId);
    return ok();
}

ScopedAStatus RadioSim::getIccCardStatus(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->getIccCardStatus(serial);
    return ok();
}

ScopedAStatus RadioSim::getImsiForApp(int32_t serial, const std::string& aid) {
    LOG_CALL << serial;
    mHal1_5->getImsiForApp(serial, aid);
    return ok();
}

ScopedAStatus RadioSim::getSimPhonebookCapacity(int32_t serial) {
    LOG_CALL << serial;
    if (mHal1_6) {
        mHal1_6->getSimPhonebookCapacity(serial);
    } else {
        respond()->getSimPhonebookCapacityResponse(notSupported(serial), {});
    }
    return ok();
}

ScopedAStatus RadioSim::getSimPhonebookRecords(int32_t serial) {
    LOG_CALL << serial;
    if (mHal1_6) {
        mHal1_6->getSimPhonebookRecords(serial);
    } else {
        respond()->getSimPhonebookRecordsResponse(notSupported(serial));
    }
    return ok();
}

ScopedAStatus RadioSim::iccCloseLogicalChannel(int32_t serial, int32_t channelId) {
    LOG_CALL << serial;
    mHal1_5->iccCloseLogicalChannel(serial, channelId);
    return ok();
}

ScopedAStatus RadioSim::iccCloseLogicalChannelWithSessionInfo(int32_t serial,
                                                        const aidl::SessionInfo& /*SessionInfo*/) {
    LOG_CALL << serial;
    LOG(ERROR) << " iccCloseLogicalChannelWithSessionInfo is unsupported by HIDL HALs";
    respond()->iccCloseLogicalChannelWithSessionInfoResponse(notSupported(serial));
    return ok();
}

ScopedAStatus RadioSim::iccIoForApp(int32_t serial, const aidl::IccIo& iccIo) {
    LOG_CALL << serial;
    mHal1_5->iccIOForApp(serial, toHidl(iccIo));
    return ok();
}

ScopedAStatus RadioSim::iccOpenLogicalChannel(int32_t serial, const std::string& aid, int32_t p2) {
    LOG_CALL << serial;
    mHal1_5->iccOpenLogicalChannel(serial, aid, p2);
    return ok();
}

ScopedAStatus RadioSim::iccTransmitApduBasicChannel(int32_t serial, const aidl::SimApdu& message) {
    LOG_CALL << serial;
    mHal1_5->iccTransmitApduBasicChannel(serial, toHidl(message));
    return ok();
}

ScopedAStatus RadioSim::iccTransmitApduLogicalChannel(int32_t serial,
                                                      const aidl::SimApdu& message) {
    LOG_CALL << serial;
    mHal1_5->iccTransmitApduLogicalChannel(serial, toHidl(message));
    return ok();
}

ScopedAStatus RadioSim::reportStkServiceIsRunning(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->reportStkServiceIsRunning(serial);
    return ok();
}

ScopedAStatus RadioSim::requestIccSimAuthentication(  //
        int32_t serial, int32_t authContext, const std::string& authData, const std::string& aid) {
    LOG_CALL << serial;
    mHal1_5->requestIccSimAuthentication(serial, authContext, authData, aid);
    return ok();
}

ScopedAStatus RadioSim::responseAcknowledgement() {
    LOG_CALL;
    mHal1_5->responseAcknowledgement();
    return ok();
}

ScopedAStatus RadioSim::sendEnvelope(int32_t serial, const std::string& command) {
    LOG_CALL << serial;
    mHal1_5->sendEnvelope(serial, command);
    return ok();
}

ScopedAStatus RadioSim::sendEnvelopeWithStatus(int32_t serial, const std::string& contents) {
    LOG_CALL << serial;
    mHal1_5->sendEnvelopeWithStatus(serial, contents);
    return ok();
}

ScopedAStatus RadioSim::sendTerminalResponseToSim(int32_t serial,
                                                  const std::string& commandResponse) {
    LOG_CALL << serial;
    mHal1_5->sendTerminalResponseToSim(serial, commandResponse);
    return ok();
}

ScopedAStatus RadioSim::setAllowedCarriers(  //
        int32_t serial, const aidl::CarrierRestrictions& carriers, aidl::SimLockMultiSimPolicy mp) {
    LOG_CALL << serial;
    mHal1_5->setAllowedCarriers_1_4(serial, toHidl(carriers), V1_4::SimLockMultiSimPolicy(mp));
    return ok();
}

ScopedAStatus RadioSim::setCarrierInfoForImsiEncryption(
        int32_t serial, const aidl::ImsiEncryptionInfo& imsiEncryptionInfo) {
    LOG_CALL << serial;
    if (mHal1_6) {
        mHal1_6->setCarrierInfoForImsiEncryption_1_6(serial, toHidl_1_6(imsiEncryptionInfo));
    } else {
        mHal1_5->setCarrierInfoForImsiEncryption(serial, toHidl(imsiEncryptionInfo));
    }
    return ok();
}

ScopedAStatus RadioSim::setCdmaSubscriptionSource(int32_t serial,
                                                  aidl::CdmaSubscriptionSource cdmaSub) {
    LOG_CALL << serial;
    mHal1_5->setCdmaSubscriptionSource(serial, V1_0::CdmaSubscriptionSource(cdmaSub));
    return ok();
}

ScopedAStatus RadioSim::setFacilityLockForApp(  //
        int32_t serial, const std::string& facility, bool lockState, const std::string& password,
        int32_t serviceClass, const std::string& appId) {
    LOG_CALL << serial;
    mHal1_5->setFacilityLockForApp(serial, facility, lockState, password, serviceClass, appId);
    return ok();
}

ScopedAStatus RadioSim::setResponseFunctions(
        const std::shared_ptr<aidl::IRadioSimResponse>& response,
        const std::shared_ptr<aidl::IRadioSimIndication>& indication) {
    LOG_CALL << response << ' ' << indication;
    mCallbackManager->setResponseFunctions(response, indication);
    return ok();
}

ScopedAStatus RadioSim::setSimCardPower(int32_t serial, aidl::CardPowerState powerUp) {
    LOG_CALL << serial;
    if (mHal1_6) {
        mHal1_6->setSimCardPower_1_6(serial, V1_1::CardPowerState(powerUp));
    } else {
        mHal1_5->setSimCardPower_1_1(serial, V1_1::CardPowerState(powerUp));
    }
    return ok();
}

ScopedAStatus RadioSim::setUiccSubscription(int32_t serial, const aidl::SelectUiccSub& uiccSub) {
    LOG_CALL << serial;
    mHal1_5->setUiccSubscription(serial, toHidl(uiccSub));
    return ok();
}

ScopedAStatus RadioSim::supplyIccPin2ForApp(int32_t serial, const std::string& pin2,
                                            const std::string& aid) {
    LOG_CALL << serial;
    mHal1_5->supplyIccPin2ForApp(serial, pin2, aid);
    return ok();
}

ScopedAStatus RadioSim::supplyIccPinForApp(int32_t serial, const std::string& pin,
                                           const std::string& aid) {
    LOG_CALL << serial;
    mHal1_5->supplyIccPinForApp(serial, pin, aid);
    return ok();
}

ScopedAStatus RadioSim::supplyIccPuk2ForApp(int32_t serial, const std::string& puk2,
                                            const std::string& pin2, const std::string& aid) {
    LOG_CALL << serial;
    mHal1_5->supplyIccPuk2ForApp(serial, puk2, pin2, aid);
    return ok();
}

ScopedAStatus RadioSim::supplyIccPukForApp(int32_t serial, const std::string& puk,
                                           const std::string& pin, const std::string& aid) {
    LOG_CALL << serial;
    mHal1_5->supplyIccPukForApp(serial, puk, pin, aid);
    return ok();
}

ScopedAStatus RadioSim::supplySimDepersonalization(int32_t serial, aidl::PersoSubstate pss,
                                                   const std::string& controlKey) {
    LOG_CALL << serial;
    mHal1_5->supplySimDepersonalization(serial, V1_5::PersoSubstate(pss), controlKey);
    return ok();
}

ScopedAStatus RadioSim::updateSimPhonebookRecords(int32_t serial,
                                                  const aidl::PhonebookRecordInfo& recordInfo) {
    LOG_CALL << serial;
    if (mHal1_6) {
        mHal1_6->updateSimPhonebookRecords(serial, toHidl(recordInfo));
    } else {
        respond()->updateSimPhonebookRecordsResponse(notSupported(serial), 0);
    }
    return ok();
}
}  // namespace android::hardware::radio::compat
