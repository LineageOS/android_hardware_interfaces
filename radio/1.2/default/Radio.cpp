/*
 * Copyright (C) 2018 The Android Open Source Project
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
#include "Radio.h"

namespace android {
namespace hardware {
namespace radio {
namespace V1_2 {
namespace implementation {

// Methods from ::android::hardware::radio::V1_0::IRadio follow.
Return<void> Radio::setResponseFunctions(
    const sp<::android::hardware::radio::V1_0::IRadioResponse>& radioResponse,
    const sp<::android::hardware::radio::V1_0::IRadioIndication>& radioIndication) {
    mRadioResponse = radioResponse;
    mRadioIndication = radioIndication;
    mRadioResponseV1_1 = ::android::hardware::radio::V1_1::IRadioResponse::castFrom(mRadioResponse)
                             .withDefault(nullptr);
    mRadioIndicationV1_1 =
        ::android::hardware::radio::V1_1::IRadioIndication::castFrom(mRadioIndication)
            .withDefault(nullptr);
    if (mRadioResponseV1_1 == nullptr || mRadioIndicationV1_1 == nullptr) {
        mRadioResponseV1_1 = nullptr;
        mRadioIndicationV1_1 = nullptr;
    }
    mRadioResponseV1_2 = ::android::hardware::radio::V1_2::IRadioResponse::castFrom(mRadioResponse)
                             .withDefault(nullptr);
    mRadioIndicationV1_2 =
        ::android::hardware::radio::V1_2::IRadioIndication::castFrom(mRadioIndication)
            .withDefault(nullptr);
    if (mRadioResponseV1_2 == nullptr || mRadioIndicationV1_2 == nullptr) {
        mRadioResponseV1_2 = nullptr;
        mRadioIndicationV1_2 = nullptr;
    }
    return Void();
}

Return<void> Radio::getIccCardStatus(int32_t serial) {
    /**
     * IRadio-defined request is called from the client and talk to the radio to get
     * IRadioResponse-defined response or/and IRadioIndication-defined indication back to the
     * client. This dummy implementation omits and replaces the design and implementation of vendor
     * codes that needs to handle the receipt of the request and the return of the response from the
     * radio; this just directly returns a dummy response back to the client.
     */

    ALOGD("Radio Request: getIccCardStatus is entering");

    if (mRadioResponse != nullptr || mRadioResponseV1_1 != nullptr ||
        mRadioResponseV1_2 != nullptr) {
        // Dummy RadioResponseInfo as part of response to return in 1.0, 1.1 and 1.2
        ::android::hardware::radio::V1_0::RadioResponseInfo info;
        info.serial = serial;
        info.type = ::android::hardware::radio::V1_0::RadioResponseType::SOLICITED;
        info.error = ::android::hardware::radio::V1_0::RadioError::NONE;
        /**
         * In IRadio and IRadioResponse 1.2, getIccCardStatus can trigger radio to return
         * getIccCardStatusResponse_1_2. In their 1.0 and 1.1, getIccCardStatus can trigger radio to
         * return getIccCardStatusResponse.
         */
        if (mRadioResponseV1_2 != nullptr) {
            // Dummy CardStatus as part of getIccCardStatusResponse_1_2 response to return
            ::android::hardware::radio::V1_2::CardStatus card_status;
            card_status.base.cardState = ::android::hardware::radio::V1_0::CardState::ABSENT;
            card_status.base.gsmUmtsSubscriptionAppIndex = 0;
            card_status.base.cdmaSubscriptionAppIndex = 0;
            mRadioResponseV1_2->getIccCardStatusResponse_1_2(info, card_status);
            ALOGD("Radio Response: getIccCardStatusResponse_1_2 is sent");
        } else if (mRadioResponseV1_1 != nullptr) {
            // Dummy CardStatus as part of getIccCardStatusResponse response to return
            ::android::hardware::radio::V1_0::CardStatus card_status_V1_0;
            card_status_V1_0.cardState = ::android::hardware::radio::V1_0::CardState::ABSENT;
            card_status_V1_0.gsmUmtsSubscriptionAppIndex = 0;
            card_status_V1_0.cdmaSubscriptionAppIndex = 0;
            mRadioResponseV1_1->getIccCardStatusResponse(info, card_status_V1_0);
            ALOGD("Radio Response: getIccCardStatusResponse is sent");
        } else {
            // Dummy CardStatus as part of getIccCardStatusResponse response to return
            ::android::hardware::radio::V1_0::CardStatus card_status_V1_0;
            card_status_V1_0.cardState = ::android::hardware::radio::V1_0::CardState::ABSENT;
            card_status_V1_0.gsmUmtsSubscriptionAppIndex = 0;
            card_status_V1_0.cdmaSubscriptionAppIndex = 0;
            mRadioResponse->getIccCardStatusResponse(info, card_status_V1_0);
            ALOGD("Radio Response: getIccCardStatusResponse is sent");
        }
    } else {
        ALOGD("mRadioResponse, mRadioResponseV1_1, and mRadioResponseV1_2 are NULL");
    }
    return Void();
}

Return<void> Radio::supplyIccPinForApp(int32_t /* serial */, const hidl_string& /* pin */,
                                       const hidl_string& /* aid */) {
    // TODO implement
    return Void();
}

Return<void> Radio::supplyIccPukForApp(int32_t /* serial */, const hidl_string& /* puk */,
                                       const hidl_string& /* pin */, const hidl_string& /* aid */) {
    // TODO implement
    return Void();
}

Return<void> Radio::supplyIccPin2ForApp(int32_t /* serial */, const hidl_string& /* pin2 */,
                                        const hidl_string& /* aid */) {
    // TODO implement
    return Void();
}

Return<void> Radio::supplyIccPuk2ForApp(int32_t /* serial */, const hidl_string& /* puk2 */,
                                        const hidl_string& /* pin2 */,
                                        const hidl_string& /* aid */) {
    // TODO implement
    return Void();
}

Return<void> Radio::changeIccPinForApp(int32_t /* serial */, const hidl_string& /* oldPin */,
                                       const hidl_string& /* newPin */,
                                       const hidl_string& /* aid */) {
    // TODO implement
    return Void();
}

Return<void> Radio::changeIccPin2ForApp(int32_t /* serial */, const hidl_string& /* oldPin2 */,
                                        const hidl_string& /* newPin2 */,
                                        const hidl_string& /* aid */) {
    // TODO implement
    return Void();
}

Return<void> Radio::supplyNetworkDepersonalization(int32_t /* serial */,
                                                   const hidl_string& /* netPin */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getCurrentCalls(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::dial(int32_t /* serial */,
                         const ::android::hardware::radio::V1_0::Dial& /* dialInfo */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getImsiForApp(int32_t /* serial */, const hidl_string& /* aid */) {
    // TODO implement
    return Void();
}

Return<void> Radio::hangup(int32_t /* serial */, int32_t /* gsmIndex */) {
    // TODO implement
    return Void();
}

Return<void> Radio::hangupWaitingOrBackground(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::hangupForegroundResumeBackground(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::switchWaitingOrHoldingAndActive(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::conference(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::rejectCall(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getLastCallFailCause(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getSignalStrength(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getVoiceRegistrationState(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getDataRegistrationState(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getOperator(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setRadioPower(int32_t /* serial */, bool /* on */) {
    // TODO implement
    return Void();
}

Return<void> Radio::sendDtmf(int32_t /* serial */, const hidl_string& /* s */) {
    // TODO implement
    return Void();
}

Return<void> Radio::sendSms(int32_t /* serial */,
                            const ::android::hardware::radio::V1_0::GsmSmsMessage& /* message */) {
    // TODO implement
    return Void();
}

Return<void> Radio::sendSMSExpectMore(
    int32_t /* serial */, const ::android::hardware::radio::V1_0::GsmSmsMessage& /* message */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setupDataCall(
    int32_t /* serial */, ::android::hardware::radio::V1_0::RadioTechnology /* radioTechnology */,
    const ::android::hardware::radio::V1_0::DataProfileInfo& /* dataProfileInfo */,
    bool /* modemCognitive */, bool /* roamingAllowed */, bool /* isRoaming */) {
    // TODO implement
    return Void();
}

Return<void> Radio::iccIOForApp(int32_t /* serial */,
                                const ::android::hardware::radio::V1_0::IccIo& /* iccIo */) {
    // TODO implement
    return Void();
}

Return<void> Radio::sendUssd(int32_t /* serial */, const hidl_string& /* ussd */) {
    // TODO implement
    return Void();
}

Return<void> Radio::cancelPendingUssd(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getClir(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setClir(int32_t /* serial */, int32_t /* status */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getCallForwardStatus(
    int32_t /* serial */, const ::android::hardware::radio::V1_0::CallForwardInfo& /* callInfo */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setCallForward(
    int32_t /* serial */, const ::android::hardware::radio::V1_0::CallForwardInfo& /* callInfo */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getCallWaiting(int32_t /* serial */, int32_t /* serviceClass */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setCallWaiting(int32_t /* serial */, bool /* enable */,
                                   int32_t /* serviceClass */) {
    // TODO implement
    return Void();
}

Return<void> Radio::acknowledgeLastIncomingGsmSms(
    int32_t /* serial */, bool /* success */,
    ::android::hardware::radio::V1_0::SmsAcknowledgeFailCause /* cause */) {
    // TODO implement
    return Void();
}

Return<void> Radio::acceptCall(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::deactivateDataCall(int32_t /* serial */, int32_t /* cid */,
                                       bool /* reasonRadioShutDown */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getFacilityLockForApp(int32_t /* serial */, const hidl_string& /* facility */,
                                          const hidl_string& /* password */,
                                          int32_t /* serviceClass */,
                                          const hidl_string& /* appId */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setFacilityLockForApp(int32_t /* serial */, const hidl_string& /* facility */,
                                          bool /* lockState */, const hidl_string& /* password */,
                                          int32_t /* serviceClass */,
                                          const hidl_string& /* appId */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setBarringPassword(int32_t /* serial */, const hidl_string& /* facility */,
                                       const hidl_string& /* oldPassword */,
                                       const hidl_string& /* newPassword */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getNetworkSelectionMode(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setNetworkSelectionModeAutomatic(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setNetworkSelectionModeManual(int32_t /* serial */,
                                                  const hidl_string& /* operatorNumeric */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getAvailableNetworks(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::startDtmf(int32_t /* serial */, const hidl_string& /* s */) {
    // TODO implement
    return Void();
}

Return<void> Radio::stopDtmf(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getBasebandVersion(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::separateConnection(int32_t /* serial */, int32_t /* gsmIndex */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setMute(int32_t /* serial */, bool /* enable */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getMute(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getClip(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getDataCallList(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setSuppServiceNotifications(int32_t /* serial */, bool /* enable */) {
    // TODO implement
    return Void();
}

Return<void> Radio::writeSmsToSim(
    int32_t /* serial */,
    const ::android::hardware::radio::V1_0::SmsWriteArgs& /* smsWriteArgs */) {
    // TODO implement
    return Void();
}

Return<void> Radio::deleteSmsOnSim(int32_t /* serial */, int32_t /* index */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setBandMode(int32_t /* serial */,
                                ::android::hardware::radio::V1_0::RadioBandMode /* mode */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getAvailableBandModes(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::sendEnvelope(int32_t /* serial */, const hidl_string& /* command */) {
    // TODO implement
    return Void();
}

Return<void> Radio::sendTerminalResponseToSim(int32_t /* serial */,
                                              const hidl_string& /* commandResponse */) {
    // TODO implement
    return Void();
}

Return<void> Radio::handleStkCallSetupRequestFromSim(int32_t /* serial */, bool /* accept */) {
    // TODO implement
    return Void();
}

Return<void> Radio::explicitCallTransfer(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setPreferredNetworkType(
    int32_t /* serial */, ::android::hardware::radio::V1_0::PreferredNetworkType /* nwType */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getPreferredNetworkType(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getNeighboringCids(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setLocationUpdates(int32_t /* serial */, bool /* enable */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setCdmaSubscriptionSource(
    int32_t /* serial */, ::android::hardware::radio::V1_0::CdmaSubscriptionSource /* cdmaSub */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setCdmaRoamingPreference(
    int32_t /* serial */, ::android::hardware::radio::V1_0::CdmaRoamingType /* type */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getCdmaRoamingPreference(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setTTYMode(int32_t /* serial */,
                               ::android::hardware::radio::V1_0::TtyMode /* mode */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getTTYMode(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setPreferredVoicePrivacy(int32_t /* serial */, bool /* enable */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getPreferredVoicePrivacy(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::sendCDMAFeatureCode(int32_t /* serial */,
                                        const hidl_string& /* featureCode */) {
    // TODO implement
    return Void();
}

Return<void> Radio::sendBurstDtmf(int32_t /* serial */, const hidl_string& /* dtmf*/,
                                  int32_t /*on*/, int32_t /*off */) {
    // TODO implement
    return Void();
}

Return<void> Radio::sendCdmaSms(int32_t /* serial */,
                                const ::android::hardware::radio::V1_0::CdmaSmsMessage& /* sms */) {
    // TODO implement
    return Void();
}

Return<void> Radio::acknowledgeLastIncomingCdmaSms(
    int32_t /* serial */, const ::android::hardware::radio::V1_0::CdmaSmsAck& /* smsAck */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getGsmBroadcastConfig(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setGsmBroadcastConfig(
    int32_t /* serial */,
    const hidl_vec<::android::hardware::radio::V1_0::GsmBroadcastSmsConfigInfo>& /* configInfo */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setGsmBroadcastActivation(int32_t /* serial */, bool /* activate */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getCdmaBroadcastConfig(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setCdmaBroadcastConfig(
    int32_t /* serial */,
    const hidl_vec<
        ::android::hardware::radio::V1_0::CdmaBroadcastSmsConfigInfo>& /* configInfo */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setCdmaBroadcastActivation(int32_t /* serial */, bool /* activate */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getCDMASubscription(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::writeSmsToRuim(
    int32_t /* serial */, const ::android::hardware::radio::V1_0::CdmaSmsWriteArgs& /* cdmaSms */) {
    // TODO implement
    return Void();
}

Return<void> Radio::deleteSmsOnRuim(int32_t /* serial */, int32_t /* index */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getDeviceIdentity(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::exitEmergencyCallbackMode(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getSmscAddress(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setSmscAddress(int32_t /* serial */, const hidl_string& /* smsc */) {
    // TODO implement
    return Void();
}

Return<void> Radio::reportSmsMemoryStatus(int32_t /* serial */, bool /* available */) {
    // TODO implement
    return Void();
}

Return<void> Radio::reportStkServiceIsRunning(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getCdmaSubscriptionSource(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::requestIsimAuthentication(int32_t /* serial */,
                                              const hidl_string& /* challenge */) {
    // TODO implement
    return Void();
}

Return<void> Radio::acknowledgeIncomingGsmSmsWithPdu(int32_t /* serial */, bool /* success */,
                                                     const hidl_string& /* ackPdu */) {
    // TODO implement
    return Void();
}

Return<void> Radio::sendEnvelopeWithStatus(int32_t /* serial */,
                                           const hidl_string& /* contents */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getVoiceRadioTechnology(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getCellInfoList(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setCellInfoListRate(int32_t /* serial */, int32_t /*rate */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setInitialAttachApn(
    int32_t /* serial */,
    const ::android::hardware::radio::V1_0::DataProfileInfo& /* dataProfileInfo */,
    bool /* modemCognitive */, bool /* isRoaming */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getImsRegistrationState(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::sendImsSms(
    int32_t /* serial */, const ::android::hardware::radio::V1_0::ImsSmsMessage& /* message */) {
    // TODO implement
    return Void();
}

Return<void> Radio::iccTransmitApduBasicChannel(
    int32_t /* serial */, const ::android::hardware::radio::V1_0::SimApdu& /* message */) {
    // TODO implement
    return Void();
}

Return<void> Radio::iccOpenLogicalChannel(int32_t /* serial */, const hidl_string& /* aid*/,
                                          int32_t /*p2 */) {
    // TODO implement
    return Void();
}

Return<void> Radio::iccCloseLogicalChannel(int32_t /* serial */, int32_t /* channelId */) {
    // TODO implement
    return Void();
}

Return<void> Radio::iccTransmitApduLogicalChannel(
    int32_t /* serial */, const ::android::hardware::radio::V1_0::SimApdu& /* message */) {
    // TODO implement
    return Void();
}

Return<void> Radio::nvReadItem(int32_t /* serial */,
                               ::android::hardware::radio::V1_0::NvItem /* itemId */) {
    // TODO implement
    return Void();
}

Return<void> Radio::nvWriteItem(int32_t /* serial */,
                                const ::android::hardware::radio::V1_0::NvWriteItem& /* item */) {
    // TODO implement
    return Void();
}

Return<void> Radio::nvWriteCdmaPrl(int32_t /* serial */, const hidl_vec<uint8_t>& /* prl */) {
    // TODO implement
    return Void();
}

Return<void> Radio::nvResetConfig(int32_t /* serial */,
                                  ::android::hardware::radio::V1_0::ResetNvType /* resetType */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setUiccSubscription(
    int32_t /* serial */, const ::android::hardware::radio::V1_0::SelectUiccSub& /* uiccSub */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setDataAllowed(int32_t /* serial */, bool /* allow */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getHardwareConfig(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::requestIccSimAuthentication(int32_t /* serial */, int32_t /* authContext */,
                                                const hidl_string& /* authData */,
                                                const hidl_string& /* aid */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setDataProfile(
    int32_t /* serial */,
    const hidl_vec<::android::hardware::radio::V1_0::DataProfileInfo>& /* profiles */,
    bool /* isRoaming */) {
    // TODO implement
    return Void();
}

Return<void> Radio::requestShutdown(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getRadioCapability(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setRadioCapability(
    int32_t /* serial */, const ::android::hardware::radio::V1_0::RadioCapability& /* rc */) {
    // TODO implement
    return Void();
}

Return<void> Radio::startLceService(int32_t /* serial */, int32_t /* reportInterval */,
                                    bool /* pullMode */) {
    // TODO implement
    return Void();
}

Return<void> Radio::stopLceService(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::pullLceData(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getModemActivityInfo(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setAllowedCarriers(
    int32_t /* serial */, bool /* allAllowed */,
    const ::android::hardware::radio::V1_0::CarrierRestrictions& /* carriers */) {
    // TODO implement
    return Void();
}

Return<void> Radio::getAllowedCarriers(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::sendDeviceState(
    int32_t /* serial */, ::android::hardware::radio::V1_0::DeviceStateType /* deviceStateType */,
    bool /* state */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setIndicationFilter(int32_t /* serial */,
                                        hidl_bitfield<IndicationFilter> /* indicationFilter */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setSimCardPower(int32_t /* serial */, bool /* powerUp */) {
    // TODO implement
    return Void();
}

Return<void> Radio::responseAcknowledgement() {
    // TODO implement
    return Void();
}

// Methods from ::android::hardware::radio::V1_1::IRadio follow.
Return<void> Radio::setCarrierInfoForImsiEncryption(
    int32_t /* serial */,
    const ::android::hardware::radio::V1_1::ImsiEncryptionInfo& /* imsiEncryptionInfo */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setSimCardPower_1_1(
    int32_t /* serial */, ::android::hardware::radio::V1_1::CardPowerState /* powerUp */) {
    // TODO implement
    return Void();
}

Return<void> Radio::startNetworkScan(
    int32_t /* serial */,
    const ::android::hardware::radio::V1_1::NetworkScanRequest& /* request */) {
    // TODO implement
    return Void();
}

Return<void> Radio::stopNetworkScan(int32_t /* serial */) {
    // TODO implement
    return Void();
}

Return<void> Radio::startKeepalive(
    int32_t /* serial */,
    const ::android::hardware::radio::V1_1::KeepaliveRequest& /* keepalive */) {
    // TODO implement
    return Void();
}

Return<void> Radio::stopKeepalive(int32_t /* serial */, int32_t /* sessionHandle */) {
    // TODO implement
    return Void();
}

// Methods from ::android::hardware::radio::V1_2::IRadio follow.
Return<void> Radio::startNetworkScan_1_2(
    int32_t /* serial */,
    const ::android::hardware::radio::V1_2::NetworkScanRequest& /* request */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setIndicationFilter_1_2(
    int32_t /* serial */, hidl_bitfield<IndicationFilter> /* indicationFilter */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setSignalStrengthReportingCriteria(
    int32_t /* serial */, int32_t /*hysteresisMs*/, int32_t /*hysteresisDb */,
    const hidl_vec<int32_t>& /* thresholdsDbm */,
    ::android::hardware::radio::V1_2::AccessNetwork /* accessNetwork */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setLinkCapacityReportingCriteria(
    int32_t /* serial */, int32_t /*hysteresisMs*/, int32_t /*hysteresisDlKbps*/,
    int32_t /*hysteresisUlKbps */, const hidl_vec<int32_t>& /* thresholdsDownlinkKbps */,
    const hidl_vec<int32_t>& /* thresholdsUplinkKbps */,
    ::android::hardware::radio::V1_2::AccessNetwork /* accessNetwork */) {
    // TODO implement
    return Void();
}

Return<void> Radio::setupDataCall_1_2(
    int32_t /* serial */, ::android::hardware::radio::V1_2::AccessNetwork /* accessNetwork */,
    const ::android::hardware::radio::V1_0::DataProfileInfo& /* dataProfileInfo */,
    bool /* modemCognitive */, bool /* roamingAllowed */, bool /* isRoaming */,
    ::android::hardware::radio::V1_2::DataRequestReason /* reason */,
    const hidl_vec<hidl_string>& /* addresses */, const hidl_vec<hidl_string>& /* dnses */) {
    // TODO implement
    return Void();
}

Return<void> Radio::deactivateDataCall_1_2(
    int32_t /* serial */, int32_t /* cid */,
    ::android::hardware::radio::V1_2::DataRequestReason /* reason */) {
    // TODO implement
    return Void();
}

}  // namespace implementation
}  // namespace V1_2
}  // namespace radio
}  // namespace hardware
}  // namespace android
