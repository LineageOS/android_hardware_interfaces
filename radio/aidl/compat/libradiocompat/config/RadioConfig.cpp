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

#include <libradiocompat/RadioConfig.h>

#include "commonStructs.h"
#include "debug.h"
#include "structs.h"

#include "collections.h"

#define RADIO_MODULE "Config"

namespace android::hardware::radio::compat {

using ::ndk::ScopedAStatus;
namespace aidl = ::aidl::android::hardware::radio::config;
constexpr auto ok = &ScopedAStatus::ok;

RadioConfig::RadioConfig(sp<config::V1_1::IRadioConfig> hidlHal)
    : mHal1_1(hidlHal),
      mHal1_3(config::V1_3::IRadioConfig::castFrom(hidlHal)),
      mRadioConfigResponse(sp<RadioConfigResponse>::make()),
      mRadioConfigIndication(sp<RadioConfigIndication>::make()) {}

std::shared_ptr<aidl::IRadioConfigResponse> RadioConfig::respond() {
    return mRadioConfigResponse->respond();
}

ScopedAStatus RadioConfig::getHalDeviceCapabilities(int32_t serial) {
    LOG_CALL << serial;
    if (mHal1_3) {
        mHal1_3->getHalDeviceCapabilities(serial);
    } else {
        respond()->getHalDeviceCapabilitiesResponse(notSupported(serial), false);
    }
    return ok();
}

ScopedAStatus RadioConfig::getNumOfLiveModems(int32_t serial) {
    LOG_CALL << serial;
    mHal1_1->getModemsConfig(serial);
    return ok();
}

ScopedAStatus RadioConfig::getPhoneCapability(int32_t serial) {
    LOG_CALL << serial;
    mHal1_1->getPhoneCapability(serial);
    return ok();
}

ScopedAStatus RadioConfig::getSimultaneousCallingSupport(int32_t serial) {
    LOG_CALL << serial;
    LOG(ERROR) << " getSimultaneousCallingSupport is unsupported by HIDL HALs";
    respond()->getSimultaneousCallingSupportResponse(notSupported(serial), {});
    return ok();
}

ScopedAStatus RadioConfig::getSimSlotsStatus(int32_t serial) {
    LOG_CALL << serial;
    mHal1_1->getSimSlotsStatus(serial);
    return ok();
}

ScopedAStatus RadioConfig::setNumOfLiveModems(int32_t serial, int8_t numOfLiveModems) {
    LOG_CALL << serial;
    mHal1_1->setModemsConfig(serial, {static_cast<uint8_t>(numOfLiveModems)});
    return ok();
}

ScopedAStatus RadioConfig::setPreferredDataModem(int32_t serial, int8_t modemId) {
    LOG_CALL << serial;
    mHal1_1->setPreferredDataModem(serial, modemId);
    return ok();
}

ScopedAStatus RadioConfig::setResponseFunctions(
        const std::shared_ptr<aidl::IRadioConfigResponse>& radioConfigResponse,
        const std::shared_ptr<aidl::IRadioConfigIndication>& radioConfigIndication) {
    LOG_CALL << radioConfigResponse << ' ' << radioConfigIndication;

    CHECK(radioConfigResponse);
    CHECK(radioConfigIndication);

    mRadioConfigResponse->setResponseFunction(radioConfigResponse);
    mRadioConfigIndication->setResponseFunction(radioConfigIndication);
    mHal1_1->setResponseFunctions(mRadioConfigResponse, mRadioConfigIndication).assertOk();

    return ok();
}

ScopedAStatus RadioConfig::setSimSlotsMapping(  //
        int32_t serial, const std::vector<aidl::SlotPortMapping>& slotMap) {
    LOG_CALL << serial;
    mHal1_1->setSimSlotsMapping(serial, toHidl(slotMap));
    return ok();
}

}  // namespace android::hardware::radio::compat
