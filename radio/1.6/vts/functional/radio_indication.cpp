/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <radio_hidl_hal_utils_v1_6.h>

RadioIndication_v1_6::RadioIndication_v1_6(RadioHidlTest_v1_6& parent) : parent_v1_6(parent) {}

/* 1.6 Apis */
Return<void> RadioIndication_v1_6::dataCallListChanged_1_6(
        RadioIndicationType /*type*/,
        const hidl_vec<android::hardware::radio::V1_6::SetupDataCallResult>& /*dcList*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::unthrottleApn(RadioIndicationType /*type*/,
                                                 const ::android::hardware::hidl_string& /*apn*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::currentPhysicalChannelConfigs_1_6(
        RadioIndicationType /*type*/,
        const ::android::hardware::hidl_vec<
                ::android::hardware::radio::V1_6::PhysicalChannelConfig>& /*configs*/) {
    return Void();
}

/* 1.5 Apis */
Return<void> RadioIndication_v1_6::uiccApplicationsEnablementChanged(RadioIndicationType /*type*/,
                                                                     bool /*enabled*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::registrationFailed(
        RadioIndicationType /*type*/,
        const ::android::hardware::radio::V1_5::CellIdentity& /*cellIdentity*/,
        const hidl_string& /*chosenPlmn*/,
        ::android::hardware::hidl_bitfield<::android::hardware::radio::V1_5::Domain> /*domain*/,
        int32_t /*causeCode*/, int32_t /*additionalCauseCode*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::barringInfoChanged(
        RadioIndicationType /*type*/,
        const ::android::hardware::radio::V1_5::CellIdentity& /*cellIdentity*/,
        const hidl_vec<::android::hardware::radio::V1_5::BarringInfo>& /*barringInfos*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::networkScanResult_1_5(
        RadioIndicationType /*type*/,
        const ::android::hardware::radio::V1_5::NetworkScanResult& /*result*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::cellInfoList_1_5(
        RadioIndicationType /*type*/,
        const hidl_vec<::android::hardware::radio::V1_5::CellInfo>& /*records*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::dataCallListChanged_1_5(
        RadioIndicationType /*type*/,
        const hidl_vec<android::hardware::radio::V1_5::SetupDataCallResult>& /*dcList*/) {
    return Void();
}

/* 1.4 Apis */
Return<void> RadioIndication_v1_6::currentPhysicalChannelConfigs_1_4(
        RadioIndicationType /*type*/,
        const ::android::hardware::hidl_vec<
                ::android::hardware::radio::V1_4::PhysicalChannelConfig>& /*configs*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::networkScanResult_1_4(
        RadioIndicationType /*type*/,
        const ::android::hardware::radio::V1_4::NetworkScanResult& /*result*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::cellInfoList_1_4(
        RadioIndicationType /*type*/,
        const ::android::hardware::hidl_vec<
                ::android::hardware::radio::V1_4::CellInfo>& /*records*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::currentEmergencyNumberList(
        RadioIndicationType /*type*/,
        const ::android::hardware::hidl_vec<EmergencyNumber>& /*emergencyNumberList*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::dataCallListChanged_1_4(
        RadioIndicationType /*type*/,
        const ::android::hardware::hidl_vec<android::hardware::radio::V1_4::SetupDataCallResult>&
        /*dcList*/) {
    return Void();
}

/* 1.2 Apis */
Return<void> RadioIndication_v1_6::networkScanResult_1_2(
        RadioIndicationType /*type*/,
        const ::android::hardware::radio::V1_2::NetworkScanResult& /*result*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::cellInfoList_1_2(
        RadioIndicationType /*type*/,
        const ::android::hardware::hidl_vec<
                ::android::hardware::radio::V1_2::CellInfo>& /*records*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::currentLinkCapacityEstimate(
        RadioIndicationType /*type*/,
        const ::android::hardware::radio::V1_2::LinkCapacityEstimate& /*lce*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::currentLinkCapacityEstimate_1_6(
        RadioIndicationType /*type*/,
        const ::android::hardware::radio::V1_6::LinkCapacityEstimate& /*lce*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::currentPhysicalChannelConfigs(
        RadioIndicationType /*type*/,
        const ::android::hardware::hidl_vec<
                ::android::hardware::radio::V1_2::PhysicalChannelConfig>& /*configs*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::currentSignalStrength_1_2(
        RadioIndicationType /*type*/,
        const ::android::hardware::radio::V1_2::SignalStrength& /*signalStrength*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::currentSignalStrength_1_4(
        RadioIndicationType /*type*/,
        const ::android::hardware::radio::V1_4::SignalStrength& /*signalStrength*/) {
    return Void();
}

/* 1.1 Apis */
Return<void> RadioIndication_v1_6::carrierInfoForImsiEncryption(RadioIndicationType /*info*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::networkScanResult(
        RadioIndicationType /*type*/,
        const ::android::hardware::radio::V1_1::NetworkScanResult& /*result*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::keepaliveStatus(RadioIndicationType /*type*/,
                                                   const KeepaliveStatus& /*status*/) {
    return Void();
}

/* 1.0 Apis */
Return<void> RadioIndication_v1_6::radioStateChanged(RadioIndicationType /*type*/,
                                                     RadioState /*radioState*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::callStateChanged(RadioIndicationType /*type*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::networkStateChanged(RadioIndicationType /*type*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::newSms(RadioIndicationType /*type*/,
                                          const ::android::hardware::hidl_vec<uint8_t>& /*pdu*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::newSmsStatusReport(
        RadioIndicationType /*type*/, const ::android::hardware::hidl_vec<uint8_t>& /*pdu*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::newSmsOnSim(RadioIndicationType /*type*/,
                                               int32_t /*recordNumber*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::onUssd(RadioIndicationType /*type*/, UssdModeType /*modeType*/,
                                          const ::android::hardware::hidl_string& /*msg*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::nitzTimeReceived(
        RadioIndicationType /*type*/, const ::android::hardware::hidl_string& /*nitzTime*/,
        uint64_t /*receivedTime*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::currentSignalStrength(
        RadioIndicationType /*type*/,
        const ::android::hardware::radio::V1_0::SignalStrength& /*signalStrength*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::dataCallListChanged(
        RadioIndicationType /*type*/,
        const ::android::hardware::hidl_vec<android::hardware::radio::V1_0::SetupDataCallResult>&
        /*dcList*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::suppSvcNotify(RadioIndicationType /*type*/,
                                                 const SuppSvcNotification& /*suppSvc*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::stkSessionEnd(RadioIndicationType /*type*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::stkProactiveCommand(
        RadioIndicationType /*type*/, const ::android::hardware::hidl_string& /*cmd*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::stkEventNotify(RadioIndicationType /*type*/,
                                                  const ::android::hardware::hidl_string& /*cmd*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::stkCallSetup(RadioIndicationType /*type*/, int64_t /*timeout*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::simSmsStorageFull(RadioIndicationType /*type*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::simRefresh(RadioIndicationType /*type*/,
                                              const SimRefreshResult& /*refreshResult*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::callRing(RadioIndicationType /*type*/, bool /*isGsm*/,
                                            const CdmaSignalInfoRecord& /*record*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::simStatusChanged(RadioIndicationType /*type*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::cdmaNewSms(RadioIndicationType /*type*/,
                                              const CdmaSmsMessage& /*msg*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::newBroadcastSms(
        RadioIndicationType /*type*/, const ::android::hardware::hidl_vec<uint8_t>& /*data*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::cdmaRuimSmsStorageFull(RadioIndicationType /*type*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::restrictedStateChanged(RadioIndicationType /*type*/,
                                                          PhoneRestrictedState /*state*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::enterEmergencyCallbackMode(RadioIndicationType /*type*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::cdmaCallWaiting(RadioIndicationType /*type*/,
                                                   const CdmaCallWaiting& /*callWaitingRecord*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::cdmaOtaProvisionStatus(RadioIndicationType /*type*/,
                                                          CdmaOtaProvisionStatus /*status*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::cdmaInfoRec(RadioIndicationType /*type*/,
                                               const CdmaInformationRecords& /*records*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::indicateRingbackTone(RadioIndicationType /*type*/,
                                                        bool /*start*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::resendIncallMute(RadioIndicationType /*type*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::cdmaSubscriptionSourceChanged(
        RadioIndicationType /*type*/, CdmaSubscriptionSource /*cdmaSource*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::cdmaPrlChanged(RadioIndicationType /*type*/,
                                                  int32_t /*version*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::exitEmergencyCallbackMode(RadioIndicationType /*type*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::rilConnected(RadioIndicationType /*type*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::voiceRadioTechChanged(
        RadioIndicationType /*type*/, ::android::hardware::radio::V1_0::RadioTechnology /*rat*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::cellInfoList(
        RadioIndicationType /*type*/,
        const ::android::hardware::hidl_vec<
                ::android::hardware::radio::V1_0::CellInfo>& /*records*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::imsNetworkStateChanged(RadioIndicationType /*type*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::subscriptionStatusChanged(RadioIndicationType /*type*/,
                                                             bool /*activate*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::srvccStateNotify(RadioIndicationType /*type*/,
                                                    SrvccState /*state*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::hardwareConfigChanged(
        RadioIndicationType /*type*/,
        const ::android::hardware::hidl_vec<HardwareConfig>& /*configs*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::radioCapabilityIndication(
        RadioIndicationType /*type*/,
        const android::hardware::radio::V1_0::RadioCapability& /*rc*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::onSupplementaryServiceIndication(
        RadioIndicationType /*type*/, const StkCcUnsolSsResult& /*ss*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::stkCallControlAlphaNotify(
        RadioIndicationType /*type*/, const ::android::hardware::hidl_string& /*alpha*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::lceData(RadioIndicationType /*type*/,
                                           const LceDataInfo& /*lce*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::pcoData(RadioIndicationType /*type*/,
                                           const PcoDataInfo& /*pco*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::modemReset(RadioIndicationType /*type*/,
                                              const ::android::hardware::hidl_string& /*reason*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::currentSignalStrength_1_6(
        RadioIndicationType /*type*/,
        const ::android::hardware::radio::V1_6::SignalStrength& /*signalStrength*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::networkScanResult_1_6(
        RadioIndicationType /*type*/,
        const ::android::hardware::radio::V1_6::NetworkScanResult& /*result*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::cellInfoList_1_6(
        RadioIndicationType /*type*/,
        const ::android::hardware::hidl_vec<
                ::android::hardware::radio::V1_6::CellInfo>& /*records*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::simPhonebookChanged(
        RadioIndicationType /*type*/) {
    return Void();
}

Return<void> RadioIndication_v1_6::simPhonebookRecordsReceived(
        RadioIndicationType /*type*/,
        ::android::hardware::radio::V1_6::PbReceivedStatus /*status*/,
        const ::android::hardware::hidl_vec<
        ::android::hardware::radio::V1_6::PhonebookRecordInfo>& /*records*/) {
    return Void();
}
