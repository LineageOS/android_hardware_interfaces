/*
 * Copyright (C) 2018 The Android Open Source Project
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

#define LOG_TAG "AGnss"

#include "AGnss.h"
#include <log/log.h>

namespace android {
namespace hardware {
namespace gnss {
namespace V2_0 {
namespace implementation {

// Methods from ::android::hardware::gnss::V2_0::IAGnss follow.
Return<void> AGnss::setCallback(const sp<V2_0::IAGnssCallback>&) {
    // TODO implement
    return Void();
}

Return<bool> AGnss::dataConnClosed() {
    // TODO implement
    return bool{};
}

Return<bool> AGnss::dataConnFailed() {
    // TODO implement
    return bool{};
}

Return<bool> AGnss::setServer(V2_0::IAGnssCallback::AGnssType type, const hidl_string& hostname,
                              int32_t port) {
    ALOGD("setServer: type: %s, hostname: %s, port: %d", toString(type).c_str(), hostname.c_str(),
          port);
    return true;
}

Return<bool> AGnss::dataConnOpen(uint64_t, const hidl_string&, V2_0::IAGnss::ApnIpType) {
    // TODO implement
    return bool{};
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android
