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

#include <libradiocompat/RadioIms.h>

#include "commonStructs.h"
#include "debug.h"

#include "collections.h"

#define RADIO_MODULE "Ims"

namespace android::hardware::radio::compat {

using ::ndk::ScopedAStatus;
namespace aidl = ::aidl::android::hardware::radio::ims;
constexpr auto ok = &ScopedAStatus::ok;

std::shared_ptr<aidl::IRadioImsResponse> RadioIms::respond() {
    return mCallbackManager->response().imsCb();
}

ScopedAStatus RadioIms::setSrvccCallInfo(
        int32_t serial, const std::vector<aidl::SrvccCall>& /*srvccCalls*/) {
    LOG_CALL << serial;
    LOG(ERROR) << " setSrvccCallInfo is unsupported by HIDL HALs";
    return ok();
}
ScopedAStatus RadioIms::updateImsRegistrationInfo(
        int32_t serial, const aidl::ImsRegistration& /*imsRegistration*/) {
    LOG_CALL << serial;
    LOG(ERROR) << " updateImsRegistrationInfo is unsupported by HIDL HALs";
    return ok();
}
ScopedAStatus RadioIms::startImsTraffic(
        int32_t serial, int32_t /*token*/, aidl::ImsTrafficType /*imsTrafficType*/,
        ::aidl::android::hardware::radio::AccessNetwork /*accessNetworkType*/,
        ::aidl::android::hardware::radio::ims::ImsCall::Direction /*trafficDirection*/) {
    LOG_CALL << serial;
    LOG(ERROR) << " startImsTraffic is unsupported by HIDL HALs";
    return ok();
}
ScopedAStatus RadioIms::stopImsTraffic(int32_t serial, int32_t /*token*/) {
    LOG_CALL << serial;
    LOG(ERROR) << " stopImsTraffic is unsupported by HIDL HALs";
    return ok();
}
ScopedAStatus RadioIms::triggerEpsFallback(int32_t serial, aidl::EpsFallbackReason /*reason*/) {
    LOG_CALL << serial;
    LOG(ERROR) << " triggerEpsFallback is unsupported by HIDL HALs";
    return ok();
}
ScopedAStatus RadioIms::sendAnbrQuery(
        int32_t serial, aidl::ImsStreamType /*mediaType*/, aidl::ImsStreamDirection /*direction*/,
        int32_t /*bitsPerSecond*/) {
    LOG_CALL << serial;
    LOG(ERROR) << " sendAnbrQuery is unsupported by HIDL HALs";
    return ok();
}
ScopedAStatus RadioIms::updateImsCallStatus(
        int32_t serial, const std::vector<aidl::ImsCall>& /*imsCalls*/) {
    LOG_CALL << serial;
    LOG(ERROR) << " updateImsCallStatus is unsupported by HIDL HALs";
    return ok();
}

ScopedAStatus RadioIms::setResponseFunctions(
        const std::shared_ptr<aidl::IRadioImsResponse>& response,
        const std::shared_ptr<aidl::IRadioImsIndication>& indication) {
    LOG_CALL << response << ' ' << indication;
    mCallbackManager->setResponseFunctions(response, indication);
    return ok();
}

}  // namespace android::hardware::radio::compat
