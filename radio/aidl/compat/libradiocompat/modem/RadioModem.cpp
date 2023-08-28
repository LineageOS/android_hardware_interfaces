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

#include <libradiocompat/RadioModem.h>
#include "commonStructs.h"
#include "debug.h"
#include "structs.h"

#define RADIO_MODULE "Modem"

namespace android::hardware::radio::compat {

using ::ndk::ScopedAStatus;
namespace aidl = ::aidl::android::hardware::radio::modem;
constexpr auto ok = &ScopedAStatus::ok;

std::shared_ptr<aidl::IRadioModemResponse> RadioModem::respond() {
    return mCallbackManager->response().modemCb();
}

ScopedAStatus RadioModem::enableModem(int32_t serial, bool on) {
    LOG_CALL << serial;
    mHal1_5->enableModem(serial, on);
    return ok();
}

ScopedAStatus RadioModem::getBasebandVersion(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->getBasebandVersion(serial);
    return ok();
}

ScopedAStatus RadioModem::getDeviceIdentity(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->getDeviceIdentity(serial);
    return ok();
}

ScopedAStatus RadioModem::getImei(int32_t serial) {
    LOG_CALL << serial;
    LOG(ERROR) << " getImei is unsupported by HIDL HALs";
    respond()->getImeiResponse(notSupported(serial), {});
    return ok();
}

ScopedAStatus RadioModem::getHardwareConfig(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->getHardwareConfig(serial);
    return ok();
}

ScopedAStatus RadioModem::getModemActivityInfo(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->getModemActivityInfo(serial);
    return ok();
}

ScopedAStatus RadioModem::getModemStackStatus(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->getModemStackStatus(serial);
    return ok();
}

ScopedAStatus RadioModem::getRadioCapability(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->getRadioCapability(serial);
    return ok();
}

ScopedAStatus RadioModem::nvReadItem(int32_t serial, aidl::NvItem itemId) {
    LOG_CALL << serial;
    mHal1_5->nvReadItem(serial, V1_0::NvItem(itemId));
    return ok();
}

ScopedAStatus RadioModem::nvResetConfig(int32_t serial, aidl::ResetNvType resetType) {
    LOG_CALL << serial;
    mHal1_5->nvResetConfig(serial, V1_0::ResetNvType(resetType));
    return ok();
}

ScopedAStatus RadioModem::nvWriteCdmaPrl(int32_t serial, const std::vector<uint8_t>& prl) {
    LOG_CALL << serial;
    mHal1_5->nvWriteCdmaPrl(serial, prl);
    return ok();
}

ScopedAStatus RadioModem::nvWriteItem(int32_t serial, const aidl::NvWriteItem& item) {
    LOG_CALL << serial;
    mHal1_5->nvWriteItem(serial, toHidl(item));
    return ok();
}

ScopedAStatus RadioModem::requestShutdown(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->requestShutdown(serial);
    return ok();
}

ScopedAStatus RadioModem::responseAcknowledgement() {
    LOG_CALL;
    mHal1_5->responseAcknowledgement();
    return ok();
}

ScopedAStatus RadioModem::sendDeviceState(int32_t serial, aidl::DeviceStateType type, bool state) {
    LOG_CALL << serial;
    mHal1_5->sendDeviceState(serial, V1_0::DeviceStateType(type), state);
    return ok();
}

ScopedAStatus RadioModem::setRadioCapability(int32_t serial, const aidl::RadioCapability& rc) {
    LOG_CALL << serial;
    mHal1_5->setRadioCapability(serial, toHidl(rc));
    return ok();
}

ScopedAStatus RadioModem::setRadioPower(int32_t serial, bool powerOn, bool forEmergencyCall,
                                        bool preferredForEmergencyCall) {
    LOG_CALL << serial;
    if (mHal1_6) {
        mHal1_6->setRadioPower_1_6(serial, powerOn, forEmergencyCall, preferredForEmergencyCall);
    } else {
        mHal1_5->setRadioPower_1_5(serial, powerOn, forEmergencyCall, preferredForEmergencyCall);
    }
    return ok();
}

ScopedAStatus RadioModem::setResponseFunctions(
        const std::shared_ptr<aidl::IRadioModemResponse>& response,
        const std::shared_ptr<aidl::IRadioModemIndication>& indication) {
    LOG_CALL << response << ' ' << indication;
    mCallbackManager->setResponseFunctions(response, indication);
    return ok();
}

}  // namespace android::hardware::radio::compat
