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
#pragma once

#include "DriverContext.h"
#include "GuaranteedCallback.h"

#include <aidl/android/hardware/radio/data/IRadioDataIndication.h>
#include <aidl/android/hardware/radio/ims/IRadioImsIndication.h>
#include <aidl/android/hardware/radio/messaging/IRadioMessagingIndication.h>
#include <aidl/android/hardware/radio/modem/IRadioModemIndication.h>
#include <aidl/android/hardware/radio/network/IRadioNetworkIndication.h>
#include <aidl/android/hardware/radio/sim/IRadioSimIndication.h>
#include <aidl/android/hardware/radio/voice/IRadioVoiceIndication.h>
#include <android/hardware/radio/1.6/IRadioIndication.h>
#include <aidl/android/hardware/radio/modem/ImeiInfo.h>

namespace android::hardware::radio::compat {

class RadioIndication : public V1_6::IRadioIndication {
    std::shared_ptr<DriverContext> mContext;

    GuaranteedCallback<  //
            ::aidl::android::hardware::radio::data::IRadioDataIndication,
            ::aidl::android::hardware::radio::data::IRadioDataIndicationDefault, true>
            mDataCb;
    GuaranteedCallback<  //
            ::aidl::android::hardware::radio::messaging::IRadioMessagingIndication,
            ::aidl::android::hardware::radio::messaging::IRadioMessagingIndicationDefault, true>
            mMessagingCb;
    GuaranteedCallback<  //
            ::aidl::android::hardware::radio::modem::IRadioModemIndication,
            ::aidl::android::hardware::radio::modem::IRadioModemIndicationDefault, true>
            mModemCb;
    GuaranteedCallback<  //
            ::aidl::android::hardware::radio::network::IRadioNetworkIndication,
            ::aidl::android::hardware::radio::network::IRadioNetworkIndicationDefault, true>
            mNetworkCb;
    GuaranteedCallback<  //
            ::aidl::android::hardware::radio::sim::IRadioSimIndication,
            ::aidl::android::hardware::radio::sim::IRadioSimIndicationDefault, true>
            mSimCb;
    GuaranteedCallback<  //
            ::aidl::android::hardware::radio::voice::IRadioVoiceIndication,
            ::aidl::android::hardware::radio::voice::IRadioVoiceIndicationDefault, true>
            mVoiceCb;
    GuaranteedCallback<  //
            ::aidl::android::hardware::radio::ims::IRadioImsIndication,
            ::aidl::android::hardware::radio::ims::IRadioImsIndicationDefault, true>
            mImsCb;

    // IRadioIndication @ 1.0
    Return<void> radioStateChanged(V1_0::RadioIndicationType type,
                                   V1_0::RadioState radioState) override;
    Return<void> callStateChanged(V1_0::RadioIndicationType type) override;
    Return<void> networkStateChanged(V1_0::RadioIndicationType type) override;
    Return<void> newSms(V1_0::RadioIndicationType type, const hidl_vec<uint8_t>& pdu) override;
    Return<void> newSmsStatusReport(V1_0::RadioIndicationType type,
                                    const hidl_vec<uint8_t>& pdu) override;
    Return<void> newSmsOnSim(V1_0::RadioIndicationType type, int32_t recordNumber) override;
    Return<void> onUssd(V1_0::RadioIndicationType type, V1_0::UssdModeType modeType,
                        const hidl_string& msg) override;
    Return<void> nitzTimeReceived(V1_0::RadioIndicationType type, const hidl_string& nitzTime,
                                  uint64_t receivedTime) override;
    Return<void> currentSignalStrength(V1_0::RadioIndicationType type,
                                       const V1_0::SignalStrength& signalStrength) override;
    Return<void> dataCallListChanged(V1_0::RadioIndicationType type,
                                     const hidl_vec<V1_0::SetupDataCallResult>& dcList) override;
    Return<void> suppSvcNotify(V1_0::RadioIndicationType type,
                               const V1_0::SuppSvcNotification& suppSvc) override;
    Return<void> stkSessionEnd(V1_0::RadioIndicationType type) override;
    Return<void> stkProactiveCommand(V1_0::RadioIndicationType type,
                                     const hidl_string& cmd) override;
    Return<void> stkEventNotify(V1_0::RadioIndicationType type, const hidl_string& cmd) override;
    Return<void> stkCallSetup(V1_0::RadioIndicationType type, int64_t timeout) override;
    Return<void> simSmsStorageFull(V1_0::RadioIndicationType type) override;
    Return<void> simRefresh(V1_0::RadioIndicationType type,
                            const V1_0::SimRefreshResult& refreshResult) override;
    Return<void> callRing(V1_0::RadioIndicationType type, bool isGsm,
                          const V1_0::CdmaSignalInfoRecord& record) override;
    Return<void> simStatusChanged(V1_0::RadioIndicationType type) override;
    Return<void> cdmaNewSms(V1_0::RadioIndicationType type,
                            const V1_0::CdmaSmsMessage& msg) override;
    Return<void> newBroadcastSms(V1_0::RadioIndicationType type,
                                 const hidl_vec<uint8_t>& data) override;
    Return<void> cdmaRuimSmsStorageFull(V1_0::RadioIndicationType type) override;
    Return<void> restrictedStateChanged(V1_0::RadioIndicationType type,
                                        V1_0::PhoneRestrictedState state) override;
    Return<void> enterEmergencyCallbackMode(V1_0::RadioIndicationType type) override;
    Return<void> cdmaCallWaiting(V1_0::RadioIndicationType type,
                                 const V1_0::CdmaCallWaiting& callWaitingRecord) override;
    Return<void> cdmaOtaProvisionStatus(V1_0::RadioIndicationType type,
                                        V1_0::CdmaOtaProvisionStatus status) override;
    Return<void> cdmaInfoRec(V1_0::RadioIndicationType type,
                             const V1_0::CdmaInformationRecords& records) override;
    Return<void> indicateRingbackTone(V1_0::RadioIndicationType type, bool start) override;
    Return<void> resendIncallMute(V1_0::RadioIndicationType type) override;
    Return<void> cdmaSubscriptionSourceChanged(V1_0::RadioIndicationType type,
                                               V1_0::CdmaSubscriptionSource cdmaSource) override;
    Return<void> cdmaPrlChanged(V1_0::RadioIndicationType type, int32_t version) override;
    Return<void> exitEmergencyCallbackMode(V1_0::RadioIndicationType type) override;
    Return<void> rilConnected(V1_0::RadioIndicationType type) override;
    Return<void> voiceRadioTechChanged(V1_0::RadioIndicationType type,
                                       V1_0::RadioTechnology rat) override;
    Return<void> cellInfoList(V1_0::RadioIndicationType type,
                              const hidl_vec<V1_0::CellInfo>& records) override;
    Return<void> imsNetworkStateChanged(V1_0::RadioIndicationType type) override;
    Return<void> subscriptionStatusChanged(V1_0::RadioIndicationType type, bool activate) override;
    Return<void> srvccStateNotify(V1_0::RadioIndicationType type, V1_0::SrvccState state) override;
    Return<void> hardwareConfigChanged(V1_0::RadioIndicationType type,
                                       const hidl_vec<V1_0::HardwareConfig>& configs) override;
    Return<void> radioCapabilityIndication(V1_0::RadioIndicationType type,
                                           const V1_0::RadioCapability& rc) override;
    Return<void> onSupplementaryServiceIndication(V1_0::RadioIndicationType type,
                                                  const V1_0::StkCcUnsolSsResult& ss) override;
    Return<void> stkCallControlAlphaNotify(V1_0::RadioIndicationType type,
                                           const hidl_string& alpha) override;
    Return<void> lceData(V1_0::RadioIndicationType type, const V1_0::LceDataInfo& lce) override;
    Return<void> pcoData(V1_0::RadioIndicationType type, const V1_0::PcoDataInfo& pco) override;
    Return<void> modemReset(V1_0::RadioIndicationType type, const hidl_string& reason) override;

    // IRadioIndication @ 1.1
    Return<void> carrierInfoForImsiEncryption(V1_0::RadioIndicationType info) override;
    Return<void> networkScanResult(V1_0::RadioIndicationType type,
                                   const V1_1::NetworkScanResult& result) override;
    Return<void> keepaliveStatus(V1_0::RadioIndicationType type,
                                 const V1_1::KeepaliveStatus& status) override;

    // IRadioIndication @ 1.2
    Return<void> networkScanResult_1_2(V1_0::RadioIndicationType type,
                                       const V1_2::NetworkScanResult& result) override;
    Return<void> cellInfoList_1_2(V1_0::RadioIndicationType type,
                                  const hidl_vec<V1_2::CellInfo>& records) override;
    Return<void> currentLinkCapacityEstimate(V1_0::RadioIndicationType type,
                                             const V1_2::LinkCapacityEstimate& lce) override;
    Return<void> currentPhysicalChannelConfigs(
            V1_0::RadioIndicationType type,
            const hidl_vec<V1_2::PhysicalChannelConfig>& configs) override;
    Return<void> currentSignalStrength_1_2(V1_0::RadioIndicationType type,
                                           const V1_2::SignalStrength& signalStrength) override;

    // IRadioIndication @ 1.4
    Return<void> currentEmergencyNumberList(
            V1_0::RadioIndicationType type,
            const hidl_vec<V1_4::EmergencyNumber>& emergencyNumberList) override;
    Return<void> cellInfoList_1_4(V1_0::RadioIndicationType type,
                                  const hidl_vec<V1_4::CellInfo>& records) override;
    Return<void> networkScanResult_1_4(V1_0::RadioIndicationType type,
                                       const V1_4::NetworkScanResult& result) override;
    Return<void> currentPhysicalChannelConfigs_1_4(
            V1_0::RadioIndicationType type,
            const hidl_vec<V1_4::PhysicalChannelConfig>& configs) override;
    Return<void> dataCallListChanged_1_4(
            V1_0::RadioIndicationType type,
            const hidl_vec<V1_4::SetupDataCallResult>& dcList) override;
    Return<void> currentSignalStrength_1_4(V1_0::RadioIndicationType type,
                                           const V1_4::SignalStrength& signalStrength) override;

    // IRadioIndication @ 1.5
    Return<void> uiccApplicationsEnablementChanged(V1_0::RadioIndicationType type,
                                                   bool enabled) override;
    Return<void> registrationFailed(  //
            V1_0::RadioIndicationType type, const V1_5::CellIdentity& cellIdentity,
            const hidl_string& chosenPlmn, hidl_bitfield<V1_5::Domain> domain, int32_t causeCode,
            int32_t additionalCauseCode) override;
    Return<void> barringInfoChanged(  //
            V1_0::RadioIndicationType type, const V1_5::CellIdentity& cellIdentity,
            const hidl_vec<V1_5::BarringInfo>& barringInfos) override;
    Return<void> cellInfoList_1_5(V1_0::RadioIndicationType type,
                                  const hidl_vec<V1_5::CellInfo>& records) override;
    Return<void> networkScanResult_1_5(V1_0::RadioIndicationType type,
                                       const V1_5::NetworkScanResult& result) override;
    Return<void> dataCallListChanged_1_5(
            V1_0::RadioIndicationType type,
            const hidl_vec<V1_5::SetupDataCallResult>& dcList) override;

    // IRadioIndication @ 1.6
    Return<void> dataCallListChanged_1_6(
            V1_0::RadioIndicationType type,
            const hidl_vec<V1_6::SetupDataCallResult>& dcList) override;
    Return<void> unthrottleApn(V1_0::RadioIndicationType type, const hidl_string& apn) override;
    Return<void> slicingConfigChanged(V1_0::RadioIndicationType type,
                                      const V1_6::SlicingConfig& slicingConfig);
    Return<void> currentLinkCapacityEstimate_1_6(V1_0::RadioIndicationType type,
                                                 const V1_6::LinkCapacityEstimate& lce) override;
    Return<void> currentSignalStrength_1_6(V1_0::RadioIndicationType type,
                                           const V1_6::SignalStrength& signalStrength) override;
    Return<void> cellInfoList_1_6(V1_0::RadioIndicationType type,
                                  const hidl_vec<V1_6::CellInfo>& records) override;
    Return<void> networkScanResult_1_6(V1_0::RadioIndicationType type,
                                       const V1_6::NetworkScanResult& result) override;
    Return<void> currentPhysicalChannelConfigs_1_6(
            V1_0::RadioIndicationType type,
            const hidl_vec<V1_6::PhysicalChannelConfig>& configs) override;
    Return<void> simPhonebookChanged(V1_0::RadioIndicationType type) override;
    Return<void> simPhonebookRecordsReceived(
            V1_0::RadioIndicationType type, V1_6::PbReceivedStatus status,
            const hidl_vec<V1_6::PhonebookRecordInfo>& records) override;
    Return<void> onImeiMappingChanged(V1_0::RadioIndicationType type,
                                      ::aidl::android::hardware::radio::modem::ImeiInfo config);
  public:
    RadioIndication(std::shared_ptr<DriverContext> context);

    void setResponseFunction(
            std::shared_ptr<::aidl::android::hardware::radio::data::IRadioDataIndication> dataCb);
    void setResponseFunction(
            std::shared_ptr<::aidl::android::hardware::radio::messaging::IRadioMessagingIndication>
                    radioMessagingIndication);
    void setResponseFunction(
            std::shared_ptr<::aidl::android::hardware::radio::modem::IRadioModemIndication> modmCb);
    void setResponseFunction(
            std::shared_ptr<::aidl::android::hardware::radio::network::IRadioNetworkIndication> ni);
    void setResponseFunction(
            std::shared_ptr<::aidl::android::hardware::radio::sim::IRadioSimIndication> simCb);
    void setResponseFunction(
            std::shared_ptr<::aidl::android::hardware::radio::voice::IRadioVoiceIndication> voicCb);
    void setResponseFunction(
            std::shared_ptr<::aidl::android::hardware::radio::ims::IRadioImsIndication> imsCb);

    std::shared_ptr<::aidl::android::hardware::radio::data::IRadioDataIndication> dataCb();
    std::shared_ptr<::aidl::android::hardware::radio::messaging::IRadioMessagingIndication>
    messagingCb();
    std::shared_ptr<::aidl::android::hardware::radio::modem::IRadioModemIndication> modemCb();
    std::shared_ptr<::aidl::android::hardware::radio::network::IRadioNetworkIndication> networkCb();
    std::shared_ptr<::aidl::android::hardware::radio::sim::IRadioSimIndication> simCb();
    std::shared_ptr<::aidl::android::hardware::radio::voice::IRadioVoiceIndication> voiceCb();
    std::shared_ptr<::aidl::android::hardware::radio::ims::IRadioImsIndication> imsCb();
};

}  // namespace android::hardware::radio::compat
