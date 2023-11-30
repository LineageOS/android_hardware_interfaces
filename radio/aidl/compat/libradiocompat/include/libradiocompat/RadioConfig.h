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

#include "RadioConfigIndication.h"
#include "RadioConfigResponse.h"

#include <aidl/android/hardware/radio/config/BnRadioConfig.h>
#include <android/hardware/radio/config/1.2/IRadioConfigIndication.h>
#include <android/hardware/radio/config/1.3/IRadioConfig.h>
#include <android/hardware/radio/config/1.3/IRadioConfigResponse.h>

namespace android::hardware::radio::compat {

/**
 * HAL translator from HIDL IRadioConfig to AIDL IRadioConfig.
 *
 * This class wraps existing HIDL implementation (either a binder stub or real
 * class implementing the HAL) and implements AIDL HAL. It's up to the caller to
 * fetch source implementation and publish resulting HAL instance.
 */
class RadioConfig : public aidl::android::hardware::radio::config::BnRadioConfig {
    const sp<config::V1_1::IRadioConfig> mHal1_1;
    const sp<config::V1_3::IRadioConfig> mHal1_3;

    const sp<RadioConfigResponse> mRadioConfigResponse;
    const sp<RadioConfigIndication> mRadioConfigIndication;

    ::ndk::ScopedAStatus getHalDeviceCapabilities(int32_t serial) override;
    ::ndk::ScopedAStatus getNumOfLiveModems(int32_t serial) override;
    ::ndk::ScopedAStatus getPhoneCapability(int32_t serial) override;
    ::ndk::ScopedAStatus getSimultaneousCallingSupport(int32_t serial) override;
    ::ndk::ScopedAStatus getSimSlotsStatus(int32_t serial) override;
    ::ndk::ScopedAStatus setNumOfLiveModems(int32_t serial, int8_t numOfLiveModems) override;
    ::ndk::ScopedAStatus setPreferredDataModem(int32_t serial, int8_t modemId) override;
    ::ndk::ScopedAStatus setResponseFunctions(
            const std::shared_ptr<aidl::android::hardware::radio::config::IRadioConfigResponse>&
                    radioConfigResponse,
            const std::shared_ptr<aidl::android::hardware::radio::config::IRadioConfigIndication>&
                    radioConfigIndication) override;
    ::ndk::ScopedAStatus setSimSlotsMapping(
            int32_t serial,
            const std::vector<aidl::android::hardware::radio::config::SlotPortMapping>& slotMap)
            override;

  protected:
    std::shared_ptr<::aidl::android::hardware::radio::config::IRadioConfigResponse> respond();

  public:
    /**
     * Constructs AIDL IRadioConfig instance wrapping existing HIDL IRadioConfig instance.
     *
     * \param hidlHal existing HIDL IRadioConfig HAL instance
     */
    RadioConfig(sp<config::V1_1::IRadioConfig> hidlHal);
};

}  // namespace android::hardware::radio::compat
