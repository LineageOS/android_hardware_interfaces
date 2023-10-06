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

#include "RadioCompatBase.h"

#include <aidl/android/hardware/radio/sim/BnRadioSim.h>

namespace android::hardware::radio::compat {

class RadioSim : public RadioCompatBase, public aidl::android::hardware::radio::sim::BnRadioSim {
    ::ndk::ScopedAStatus areUiccApplicationsEnabled(int32_t serial) override;
    ::ndk::ScopedAStatus changeIccPin2ForApp(int32_t serial, const std::string& oldPin2,
                                             const std::string& newPin2,
                                             const std::string& aid) override;
    ::ndk::ScopedAStatus changeIccPinForApp(int32_t serial, const std::string& oldPin,
                                            const std::string& newPin,
                                            const std::string& aid) override;
    ::ndk::ScopedAStatus enableUiccApplications(int32_t serial, bool enable) override;
    ::ndk::ScopedAStatus getAllowedCarriers(int32_t serial) override;
    ::ndk::ScopedAStatus getCdmaSubscription(int32_t serial) override;
    ::ndk::ScopedAStatus getCdmaSubscriptionSource(int32_t serial) override;
    ::ndk::ScopedAStatus getFacilityLockForApp(int32_t serial, const std::string& facility,
                                               const std::string& password, int32_t serviceClass,
                                               const std::string& appId) override;
    ::ndk::ScopedAStatus getIccCardStatus(int32_t serial) override;
    ::ndk::ScopedAStatus getImsiForApp(int32_t serial, const std::string& aid) override;
    ::ndk::ScopedAStatus getSimPhonebookCapacity(int32_t serial) override;
    ::ndk::ScopedAStatus getSimPhonebookRecords(int32_t serial) override;
    ::ndk::ScopedAStatus iccCloseLogicalChannelWithSessionInfo(int32_t serial,
            const ::aidl::android::hardware::radio::sim::SessionInfo& recordInfo) override;
    ::ndk::ScopedAStatus iccIoForApp(
            int32_t serial, const ::aidl::android::hardware::radio::sim::IccIo& iccIo) override;
    ::ndk::ScopedAStatus iccOpenLogicalChannel(int32_t serial, const std::string& aid,
                                               int32_t p2) override;
    ::ndk::ScopedAStatus iccTransmitApduBasicChannel(
            int32_t serial, const ::aidl::android::hardware::radio::sim::SimApdu& message) override;
    ::ndk::ScopedAStatus iccTransmitApduLogicalChannel(
            int32_t serial, const ::aidl::android::hardware::radio::sim::SimApdu& message) override;
    ::ndk::ScopedAStatus reportStkServiceIsRunning(int32_t serial) override;
    ::ndk::ScopedAStatus requestIccSimAuthentication(int32_t serial, int32_t authContext,
                                                     const std::string& authData,
                                                     const std::string& aid) override;
    ::ndk::ScopedAStatus responseAcknowledgement() override;
    ::ndk::ScopedAStatus sendEnvelope(int32_t serial, const std::string& command) override;
    ::ndk::ScopedAStatus sendEnvelopeWithStatus(int32_t serial,
                                                const std::string& contents) override;
    ::ndk::ScopedAStatus sendTerminalResponseToSim(int32_t serial,
                                                   const std::string& commandResponse) override;
    ::ndk::ScopedAStatus setAllowedCarriers(
            int32_t serial,
            const ::aidl::android::hardware::radio::sim::CarrierRestrictions& carriers,
            ::aidl::android::hardware::radio::sim::SimLockMultiSimPolicy multiSimPolicy) override;
    ::ndk::ScopedAStatus setCarrierInfoForImsiEncryption(
            int32_t serial,
            const ::aidl::android::hardware::radio::sim::ImsiEncryptionInfo& imsiEncryptionInfo)
            override;
    ::ndk::ScopedAStatus setCdmaSubscriptionSource(
            int32_t serial,
            ::aidl::android::hardware::radio::sim::CdmaSubscriptionSource cdmaSub) override;
    ::ndk::ScopedAStatus setFacilityLockForApp(  //
            int32_t serial, const std::string& facility, bool lockState, const std::string& passwd,
            int32_t serviceClass, const std::string& appId) override;
    ::ndk::ScopedAStatus setResponseFunctions(
            const std::shared_ptr<::aidl::android::hardware::radio::sim::IRadioSimResponse>&
                    radioSimResponse,
            const std::shared_ptr<::aidl::android::hardware::radio::sim::IRadioSimIndication>&
                    radioSimIndication) override;
    ::ndk::ScopedAStatus setSimCardPower(
            int32_t serial, ::aidl::android::hardware::radio::sim::CardPowerState powerUp) override;
    ::ndk::ScopedAStatus setUiccSubscription(
            int32_t serial,
            const ::aidl::android::hardware::radio::sim::SelectUiccSub& uiccSub) override;
    ::ndk::ScopedAStatus supplyIccPin2ForApp(int32_t serial, const std::string& pin2,
                                             const std::string& aid) override;
    ::ndk::ScopedAStatus supplyIccPinForApp(int32_t serial, const std::string& pin,
                                            const std::string& aid) override;
    ::ndk::ScopedAStatus supplyIccPuk2ForApp(int32_t serial, const std::string& puk2,
                                             const std::string& pin2,
                                             const std::string& aid) override;
    ::ndk::ScopedAStatus supplyIccPukForApp(int32_t serial, const std::string& puk,
                                            const std::string& pin,
                                            const std::string& aid) override;
    ::ndk::ScopedAStatus supplySimDepersonalization(
            int32_t serial, ::aidl::android::hardware::radio::sim::PersoSubstate persoType,
            const std::string& controlKey) override;
    ::ndk::ScopedAStatus updateSimPhonebookRecords(
            int32_t serial,
            const ::aidl::android::hardware::radio::sim::PhonebookRecordInfo& recordInfo) override;

  protected:
    std::shared_ptr<::aidl::android::hardware::radio::sim::IRadioSimResponse> respond();
    ::ndk::ScopedAStatus iccCloseLogicalChannel(int32_t serial, int32_t channelId) override;

  public:
    using RadioCompatBase::RadioCompatBase;
};

}  // namespace android::hardware::radio::compat
