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

#ifndef ANDROID_HARDWARE_GNSS_V2_0_AGNSSRIL_H
#define ANDROID_HARDWARE_GNSS_V2_0_AGNSSRIL_H

#include <android/hardware/gnss/2.0/IAGnssRil.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace gnss {
namespace V2_0 {
namespace implementation {

using ::android::sp;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;

struct AGnssRil : public IAGnssRil {
    // Methods from ::android::hardware::gnss::V1_0::IAGnssRil follow.
    Return<void> setCallback(const sp<V1_0::IAGnssRilCallback>& callback) override;
    Return<void> setRefLocation(const V1_0::IAGnssRil::AGnssRefLocation& agnssReflocation) override;
    Return<bool> setSetId(V1_0::IAGnssRil::SetIDType type, const hidl_string& setid) override;
    Return<bool> updateNetworkState(bool connected, V1_0::IAGnssRil::NetworkType type,
                                    bool roaming) override;
    Return<bool> updateNetworkAvailability(bool available, const hidl_string& apn) override;

    // Methods from ::android::hardware::gnss::V2_0::IAGnssRil follow.
    Return<bool> updateNetworkState_2_0(
        const V2_0::IAGnssRil::NetworkAttributes& attributes) override;
};

}  // namespace implementation
}  // namespace V2_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_GNSS_V2_0_AGNSSRIL_H