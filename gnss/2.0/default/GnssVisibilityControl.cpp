/*
 * Copyright (C) 2019 The Android Open Source Project
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

#define LOG_TAG "GnssVisibilityControl"

#include "GnssVisibilityControl.h"
#include <log/log.h>

namespace android {
namespace hardware {
namespace gnss {
namespace visibility_control {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::gnss::visibility_control::V1_0::IGnssVisibilityControl follow.
Return<bool> GnssVisibilityControl::enableNfwLocationAccess(
        const hidl_vec<hidl_string>& proxyApps) {
    std::string os;
    bool first = true;
    for (const auto& proxyApp : proxyApps) {
        if (first) {
            first = false;
        } else {
            os += " ";
        }

        os += proxyApp;
    }

    ALOGD("enableNfwLocationAccess proxyApps: %s", os.c_str());
    return true;
}

Return<bool> GnssVisibilityControl::setCallback(const sp<V1_0::IGnssVisibilityControlCallback>&) {
    return true;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace visibility_control
}  // namespace gnss
}  // namespace hardware
}  // namespace android
