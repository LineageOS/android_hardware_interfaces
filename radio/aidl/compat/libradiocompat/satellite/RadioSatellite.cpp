/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <libradiocompat/RadioSatellite.h>

#include "commonStructs.h"
#include "debug.h"

#include "collections.h"

#define RADIO_MODULE "RadioSatellite"

namespace android::hardware::radio::compat {

using ::ndk::ScopedAStatus;
namespace aidl = ::aidl::android::hardware::radio::satellite;
constexpr auto ok = &ScopedAStatus::ok;

std::shared_ptr<aidl::IRadioSatelliteResponse> RadioSatellite::respond() {
    return mCallbackManager->response().satelliteCb();
}

ScopedAStatus RadioSatellite::responseAcknowledgement() {
    LOG(ERROR) << " responseAcknowledgement is unsupported by HIDL HALs";
    return ok();
}
ScopedAStatus RadioSatellite::getCapabilities(int32_t serial) {
    LOG_CALL << serial;
    LOG(ERROR) << " getCapabilities is unsupported by HIDL HALs";
    return ok();
}
ScopedAStatus RadioSatellite::setPower(int32_t serial, bool /*on*/) {
    LOG_CALL << serial;
    LOG(ERROR) << " setPower is unsupported by HIDL HALs";
    return ok();
}
ScopedAStatus RadioSatellite::getPowerState(int32_t serial) {
    LOG_CALL << serial;
    LOG(ERROR) << " getPowerSate is unsupported by HIDL HALs";
    return ok();
}
ScopedAStatus RadioSatellite::provisionService(
        int32_t serial, const std::string& /*imei*/, const std::string& /*msisdn*/,
        const std::string& /*imsi*/,
        const std::vector<
                ::aidl::android::hardware::radio::satellite::SatelliteFeature>& /*features*/) {
    LOG_CALL << serial;
    LOG(ERROR) << " provisionService is unsupported by HIDL HALs";
    return ok();
}
ScopedAStatus RadioSatellite::addAllowedSatelliteContacts(
        int32_t serial, const std::vector<std::string>& /*contacts*/) {
    LOG_CALL << serial;
    LOG(ERROR) << " addAllowedSatelliteContacts is unsupported by HIDL HALs";
    return ok();
}
ScopedAStatus RadioSatellite::removeAllowedSatelliteContacts(
        int32_t serial, const std::vector<std::string>& /*contacts*/) {
    LOG_CALL << serial;
    LOG(ERROR) << " removeAllowedSatelliteContacts is unsupported by HIDL HALs";
    return ok();
}
ScopedAStatus RadioSatellite::sendMessages(int32_t serial,
                                           const std::vector<std::string>& /*messages*/,
                                           const std::string& /*destination*/, double /*latitude*/,
                                           double /*longitude*/) {
    LOG_CALL << serial;
    LOG(ERROR) << " sendMessage is unsupported by HIDL HALs";
    return ok();
}
ScopedAStatus RadioSatellite::getPendingMessages(int32_t serial) {
    LOG_CALL << serial;
    LOG(ERROR) << " getPendingMessages is unsupported by HIDL HALs";
    return ok();
}
ScopedAStatus RadioSatellite::getSatelliteMode(int32_t serial) {
    LOG_CALL << serial;
    LOG(ERROR) << " getSatelliteMode is unsupported by HIDL HALs";
    return ok();
}
ScopedAStatus RadioSatellite::setIndicationFilter(int32_t serial, int32_t /*filterBitmask*/) {
    LOG_CALL << serial;
    LOG(ERROR) << " setIndicationFilter is unsupported by HIDL HALs";
    return ok();
}
ScopedAStatus RadioSatellite::startSendingSatellitePointingInfo(int32_t serial) {
    LOG_CALL << serial;
    LOG(ERROR) << " startSendingSatellitePointingInfo is unsupported by HIDL HALs";
    return ok();
}
ScopedAStatus RadioSatellite::stopSendingSatellitePointingInfo(int32_t serial) {
    LOG_CALL << serial;
    LOG(ERROR) << " stopSendingSatellitePointingInfo is unsupported by HIDL HALs";
    return ok();
}
ScopedAStatus RadioSatellite::getMaxCharactersPerTextMessage(int32_t serial) {
    LOG_CALL << serial;
    LOG(ERROR) << " getMaxCharactersPerTextMessage is unsupported by HIDL HALs";
    return ok();
}
ScopedAStatus RadioSatellite::getTimeForNextSatelliteVisibility(int32_t serial) {
    LOG_CALL << serial;
    LOG(ERROR) << " getTimeForNextSatelliteVisibility is unsupported by HIDL HALs";
    return ok();
}

ScopedAStatus RadioSatellite::setResponseFunctions(
        const std::shared_ptr<aidl::IRadioSatelliteResponse>& response,
        const std::shared_ptr<aidl::IRadioSatelliteIndication>& indication) {
    LOG_CALL << response << ' ' << indication;
    mCallbackManager->setResponseFunctions(response, indication);
    return ok();
}

}  // namespace android::hardware::radio::compat
