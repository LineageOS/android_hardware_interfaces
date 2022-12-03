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

#include <aidl/android/hardware/radio/modem/BnRadioModem.h>

namespace android::hardware::radio::compat {

class RadioModem : public RadioCompatBase,
                   public aidl::android::hardware::radio::modem::BnRadioModem {
    ::ndk::ScopedAStatus enableModem(int32_t serial, bool on) override;
    ::ndk::ScopedAStatus getBasebandVersion(int32_t serial) override;
    ::ndk::ScopedAStatus getDeviceIdentity(int32_t serial) override;
    ::ndk::ScopedAStatus getImei(int32_t serial) override;
    ::ndk::ScopedAStatus getHardwareConfig(int32_t serial) override;
    ::ndk::ScopedAStatus getModemActivityInfo(int32_t serial) override;
    ::ndk::ScopedAStatus getModemStackStatus(int32_t serial) override;
    ::ndk::ScopedAStatus getRadioCapability(int32_t serial) override;
    ::ndk::ScopedAStatus nvReadItem(
            int32_t serial, ::aidl::android::hardware::radio::modem::NvItem itemId) override;
    ::ndk::ScopedAStatus nvResetConfig(
            int32_t serial, ::aidl::android::hardware::radio::modem::ResetNvType type) override;
    ::ndk::ScopedAStatus nvWriteCdmaPrl(int32_t serial, const std::vector<uint8_t>& prl) override;
    ::ndk::ScopedAStatus nvWriteItem(
            int32_t serial, const ::aidl::android::hardware::radio::modem::NvWriteItem& i) override;
    ::ndk::ScopedAStatus requestShutdown(int32_t serial) override;
    ::ndk::ScopedAStatus responseAcknowledgement() override;
    ::ndk::ScopedAStatus sendDeviceState(
            int32_t serial, ::aidl::android::hardware::radio::modem::DeviceStateType stateType,
            bool state) override;
    ::ndk::ScopedAStatus setRadioCapability(
            int32_t s, const ::aidl::android::hardware::radio::modem::RadioCapability& rc) override;
    ::ndk::ScopedAStatus setRadioPower(int32_t serial, bool powerOn, bool forEmergencyCall,
                                       bool preferredForEmergencyCall) override;
    ::ndk::ScopedAStatus setResponseFunctions(
            const std::shared_ptr<::aidl::android::hardware::radio::modem::IRadioModemResponse>&
                    radioModemResponse,
            const std::shared_ptr<::aidl::android::hardware::radio::modem::IRadioModemIndication>&
                    radioModemIndication) override;

  protected:
    std::shared_ptr<::aidl::android::hardware::radio::modem::IRadioModemResponse> respond();

  public:
    using RadioCompatBase::RadioCompatBase;
};

}  // namespace android::hardware::radio::compat
