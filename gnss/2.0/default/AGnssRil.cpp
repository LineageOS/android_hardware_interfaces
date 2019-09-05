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

#define LOG_TAG "AGnssRil"

#include "AGnssRil.h"
#include <log/log.h>

namespace android {
namespace hardware {
namespace gnss {
namespace V2_0 {
namespace implementation {

// Methods from V1_0::IAGnssRil follow.
Return<void> AGnssRil::setCallback(const sp<V1_0::IAGnssRilCallback>&) {
    // TODO implement
    return Void();
}

Return<void> AGnssRil::setRefLocation(const V1_0::IAGnssRil::AGnssRefLocation&) {
    // TODO implement
    return Void();
}

Return<bool> AGnssRil::setSetId(V1_0::IAGnssRil::SetIDType, const hidl_string&) {
    // TODO implement
    return bool{};
}

Return<bool> AGnssRil::updateNetworkState(bool, V1_0::IAGnssRil::NetworkType, bool) {
    // TODO implement
    return bool{};
}

Return<bool> AGnssRil::updateNetworkAvailability(bool, const hidl_string&) {
    // TODO implement
    return bool{};
}

// Methods from ::android::hardware::gnss::V2_0::IAGnssRil follow.
Return<bool> AGnssRil::updateNetworkState_2_0(
    const V2_0::IAGnssRil::NetworkAttributes& attributes) {
    ALOGD("updateNetworkState_2_0 networkAttributes: %s", toString(attributes).c_str());
    return true;
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android
