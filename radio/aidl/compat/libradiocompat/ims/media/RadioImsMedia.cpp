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

#include <libradiocompat/RadioImsMedia.h>

#include "commonStructs.h"
#include "debug.h"

#include "collections.h"

#define RADIO_MODULE "ImsMedia"

namespace android::hardware::radio::compat {

using ::ndk::ScopedAStatus;
constexpr auto ok = &ScopedAStatus::ok;

ScopedAStatus RadioImsMedia::setListener(
        const std::shared_ptr<::aidl::android::hardware::radio::ims::media::
                                      IImsMediaListener>& /*in_mediaListener*/) {
    LOG(ERROR) << " setListener is unsupported by HIDL HALs";
    return ok();
}
ScopedAStatus RadioImsMedia::openSession(
        int32_t /*in_sessionId*/,
        const ::aidl::android::hardware::radio::ims::media::LocalEndPoint& /*in_localEndPoint*/,
        const ::aidl::android::hardware::radio::ims::media::RtpConfig& /*in_config*/) {
    LOG(ERROR) << " openSession is unsupported by HIDL HALs";
    return ok();
}
ScopedAStatus RadioImsMedia::closeSession(int32_t /*in_sessionId*/) {
    LOG(ERROR) << " closeSession is unsupported by HIDL HALs";
    return ok();
}

}  // namespace android::hardware::radio::compat
