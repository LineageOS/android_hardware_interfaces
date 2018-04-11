/*
 * Copyright (C) 2017 The Android Open Source Project
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

RadioIndication_v1_2::RadioIndication_v1_2(RadioHidlTest_v1_2& parent) : parent_v1_2(parent) {}

/* 1.2 Apis */
Return<void> RadioIndication_v1_2::networkScanResult_1_2(
    RadioIndicationType /*type*/,
    const ::android::hardware::radio::V1_2::NetworkScanResult& /*result*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::cellInfoList_1_2(
    RadioIndicationType /*type*/,
    const ::android::hardware::hidl_vec<::android::hardware::radio::V1_2::CellInfo>& /*records*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::currentLinkCapacityEstimate(
    RadioIndicationType /*type*/,
    const ::android::hardware::radio::V1_2::LinkCapacityEstimate& /*lce*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::currentPhysicalChannelConfigs(
    RadioIndicationType /*type*/,
    const ::android::hardware::hidl_vec<
        ::android::hardware::radio::V1_2::PhysicalChannelConfig>& /*configs*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::currentSignalStrength_1_2(
    RadioIndicationType /*type*/,
    const ::android::hardware::radio::V1_2::SignalStrength& /*signalStrength*/) {
    return Void();
}

/* 1.1 Apis */
Return<void> RadioIndication_v1_2::carrierInfoForImsiEncryption(RadioIndicationType /*info*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::networkScanResult(
    RadioIndicationType /*type*/,
    const ::android::hardware::radio::V1_1::NetworkScanResult& /*result*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::keepaliveStatus(RadioIndicationType /*type*/,
                                                   const KeepaliveStatus& /*status*/) {
    return Void();
}

/* 1.0 Apis */
Return<void> RadioIndication_v1_2::radioStateChanged(RadioIndicationType /*type*/,
                                                     RadioState /*radioState*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::callStateChanged(RadioIndicationType /*type*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::networkStateChanged(RadioIndicationType /*type*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::newSms(RadioIndicationType /*type*/,
                                          const ::android::hardware::hidl_vec<uint8_t>& /*pdu*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::newSmsStatusReport(
    RadioIndicationType /*type*/, const ::android::hardware::hidl_vec<uint8_t>& /*pdu*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::newSmsOnSim(RadioIndicationType /*type*/,
                                               int32_t /*recordNumber*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::onUssd(RadioIndicationType /*type*/, UssdModeType /*modeType*/,
                                          const ::android::hardware::hidl_string& /*msg*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::nitzTimeReceived(
    RadioIndicationType /*type*/, const ::android::hardware::hidl_string& /*nitzTime*/,
    uint64_t /*receivedTime*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::currentSignalStrength(
    RadioIndicationType /*type*/,
    const ::android::hardware::radio::V1_0::SignalStrength& /*signalStrength*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::dataCallListChanged(
    RadioIndicationType /*type*/,
    const ::android::hardware::hidl_vec<SetupDataCallResult>& /*dcList*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::suppSvcNotify(RadioIndicationType /*type*/,
                                                 const SuppSvcNotification& /*suppSvc*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::stkSessionEnd(RadioIndicationType /*type*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::stkProactiveCommand(
    RadioIndicationType /*type*/, const ::android::hardware::hidl_string& /*cmd*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::stkEventNotify(RadioIndicationType /*type*/,
                                                  const ::android::hardware::hidl_string& /*cmd*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::stkCallSetup(RadioIndicationType /*type*/, int64_t /*timeout*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::simSmsStorageFull(RadioIndicationType /*type*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::simRefresh(RadioIndicationType /*type*/,
                                              const SimRefreshResult& /*refreshResult*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::callRing(RadioIndicationType /*type*/, bool /*isGsm*/,
                                            const CdmaSignalInfoRecord& /*record*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::simStatusChanged(RadioIndicationType /*type*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::cdmaNewSms(RadioIndicationType /*type*/,
                                              const CdmaSmsMessage& /*msg*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::newBroadcastSms(
    RadioIndicationType /*type*/, const ::android::hardware::hidl_vec<uint8_t>& /*data*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::cdmaRuimSmsStorageFull(RadioIndicationType /*type*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::restrictedStateChanged(RadioIndicationType /*type*/,
                                                          PhoneRestrictedState /*state*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::enterEmergencyCallbackMode(RadioIndicationType /*type*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::cdmaCallWaiting(RadioIndicationType /*type*/,
                                                   const CdmaCallWaiting& /*callWaitingRecord*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::cdmaOtaProvisionStatus(RadioIndicationType /*type*/,
                                                          CdmaOtaProvisionStatus /*status*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::cdmaInfoRec(RadioIndicationType /*type*/,
                                               const CdmaInformationRecords& /*records*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::indicateRingbackTone(RadioIndicationType /*type*/,
                                                        bool /*start*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::resendIncallMute(RadioIndicationType /*type*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::cdmaSubscriptionSourceChanged(
    RadioIndicationType /*type*/, CdmaSubscriptionSource /*cdmaSource*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::cdmaPrlChanged(RadioIndicationType /*type*/,
                                                  int32_t /*version*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::exitEmergencyCallbackMode(RadioIndicationType /*type*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::rilConnected(RadioIndicationType /*type*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::voiceRadioTechChanged(RadioIndicationType /*type*/,
                                                         RadioTechnology /*rat*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::cellInfoList(
    RadioIndicationType /*type*/,
    const ::android::hardware::hidl_vec<::android::hardware::radio::V1_0::CellInfo>& /*records*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::imsNetworkStateChanged(RadioIndicationType /*type*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::subscriptionStatusChanged(RadioIndicationType /*type*/,
                                                             bool /*activate*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::srvccStateNotify(RadioIndicationType /*type*/,
                                                    SrvccState /*state*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::hardwareConfigChanged(
    RadioIndicationType /*type*/,
    const ::android::hardware::hidl_vec<HardwareConfig>& /*configs*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::radioCapabilityIndication(RadioIndicationType /*type*/,
                                                             const RadioCapability& /*rc*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::onSupplementaryServiceIndication(
    RadioIndicationType /*type*/, const StkCcUnsolSsResult& /*ss*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::stkCallControlAlphaNotify(
    RadioIndicationType /*type*/, const ::android::hardware::hidl_string& /*alpha*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::lceData(RadioIndicationType /*type*/,
                                           const LceDataInfo& /*lce*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::pcoData(RadioIndicationType /*type*/,
                                           const PcoDataInfo& /*pco*/) {
    return Void();
}

Return<void> RadioIndication_v1_2::modemReset(RadioIndicationType /*type*/,
                                              const ::android::hardware::hidl_string& /*reason*/) {
    return Void();
}