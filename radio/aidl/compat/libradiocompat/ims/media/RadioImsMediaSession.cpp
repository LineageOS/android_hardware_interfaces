/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include <libradiocompat/RadioImsMediaSession.h>

#include "commonStructs.h"
#include "debug.h"

#include "collections.h"

#define RADIO_MODULE "ImsMediaSession"

namespace android::hardware::radio::compat {

using ::ndk::ScopedAStatus;
namespace aidl = ::aidl::android::hardware::radio::ims::media;
constexpr auto ok = &ScopedAStatus::ok;

ScopedAStatus RadioImsMediaSession::setListener(
        const std::shared_ptr<aidl::IImsMediaSessionListener>& /*in_sessionListener*/) {
    LOG(ERROR) << " setListener is unsupported by HIDL HALs";
    return ok();
}
ScopedAStatus RadioImsMediaSession::modifySession(const aidl::RtpConfig& /*in_config*/) {
    LOG(ERROR) << " modifySession is unsupported by HIDL HALs";
    return ok();
}
ScopedAStatus RadioImsMediaSession::sendDtmf(char16_t /*in_dtmfDigit*/, int32_t /*in_duration*/) {
    LOG(ERROR) << " sendDtmf is unsupported by HIDL HALs";
    return ok();
}
ScopedAStatus RadioImsMediaSession::startDtmf(char16_t /*in_dtmfDigit*/) {
    LOG(ERROR) << " startDtmf is unsupported by HIDL HALs";
    return ok();
}
ScopedAStatus RadioImsMediaSession::stopDtmf() {
    LOG(ERROR) << " stopDtmf is unsupported by HIDL HALs";
    return ok();
}
ScopedAStatus RadioImsMediaSession::sendHeaderExtension(
        const std::vector<aidl::RtpHeaderExtension>& /*in_extensions*/) {
    LOG(ERROR) << " sendHeaderExtension is unsupported by HIDL HALs";
    return ok();
}
ScopedAStatus RadioImsMediaSession::setMediaQualityThreshold(
        const aidl::MediaQualityThreshold& /*in_threshold*/) {
    LOG(ERROR) << " setMediaQualityThreshold is unsupported by HIDL HALs";
    return ok();
}
ScopedAStatus RadioImsMediaSession::requestRtpReceptionStats(int32_t /*in_intervalMs*/) {
    LOG(ERROR) << " requestRtpReceptionStats is unsupported by HIDL HALs";
    return ok();
}
ScopedAStatus RadioImsMediaSession::adjustDelay(int32_t /*in_delayMs*/) {
    LOG(ERROR) << " adjustDelay is unsupported by HIDL HALs";
    return ok();
}
}  // namespace android::hardware::radio::compat
