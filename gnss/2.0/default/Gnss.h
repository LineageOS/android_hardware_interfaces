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

#ifndef ANDROID_HARDWARE_GNSS_V2_0_GNSS_H
#define ANDROID_HARDWARE_GNSS_V2_0_GNSS_H

#include <android/hardware/gnss/2.0/IGnss.h>
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

struct Gnss : public IGnss {
    // Methods from V1_0::IGnss follow.
    Return<bool> setCallback(const sp<V1_0::IGnssCallback>& callback) override;
    Return<bool> start() override;
    Return<bool> stop() override;
    Return<void> cleanup() override;
    Return<bool> injectTime(int64_t timeMs, int64_t timeReferenceMs,
                            int32_t uncertaintyMs) override;
    Return<bool> injectLocation(double latitudeDegrees, double longitudeDegrees,
                                float accuracyMeters) override;
    Return<void> deleteAidingData(V1_0::IGnss::GnssAidingData aidingDataFlags) override;
    Return<bool> setPositionMode(V1_0::IGnss::GnssPositionMode mode,
                                 V1_0::IGnss::GnssPositionRecurrence recurrence,
                                 uint32_t minIntervalMs, uint32_t preferredAccuracyMeters,
                                 uint32_t preferredTimeMs) override;
    Return<sp<V1_0::IAGnssRil>> getExtensionAGnssRil() override;
    Return<sp<V1_0::IGnssGeofencing>> getExtensionGnssGeofencing() override;
    Return<sp<V1_0::IAGnss>> getExtensionAGnss() override;
    Return<sp<V1_0::IGnssNi>> getExtensionGnssNi() override;
    Return<sp<V1_0::IGnssMeasurement>> getExtensionGnssMeasurement() override;
    Return<sp<V1_0::IGnssNavigationMessage>> getExtensionGnssNavigationMessage() override;
    Return<sp<V1_0::IGnssXtra>> getExtensionXtra() override;
    Return<sp<V1_0::IGnssConfiguration>> getExtensionGnssConfiguration() override;
    Return<sp<V1_0::IGnssDebug>> getExtensionGnssDebug() override;
    Return<sp<V1_0::IGnssBatching>> getExtensionGnssBatching() override;

    // Methods from V1_1::IGnss follow.
    Return<bool> setCallback_1_1(const sp<V1_1::IGnssCallback>& callback) override;
    Return<bool> setPositionMode_1_1(V1_0::IGnss::GnssPositionMode mode,
                                     V1_0::IGnss::GnssPositionRecurrence recurrence,
                                     uint32_t minIntervalMs, uint32_t preferredAccuracyMeters,
                                     uint32_t preferredTimeMs, bool lowPowerMode) override;
    Return<sp<V1_1::IGnssConfiguration>> getExtensionGnssConfiguration_1_1() override;
    Return<sp<V1_1::IGnssMeasurement>> getExtensionGnssMeasurement_1_1() override;
    Return<bool> injectBestLocation(const V1_0::GnssLocation& location) override;

    // Methods from V2_0::IGnss follow.
    Return<sp<V2_0::IAGnss>> getExtensionAGnss_2_0() override;
    Return<sp<V2_0::IAGnssRil>> getExtensionAGnssRil_2_0() override;
    Return<sp<V2_0::IGnssMeasurement>> getExtensionGnssMeasurement_2_0() override;
    Return<bool> setCallback_2_0(const sp<V2_0::IGnssCallback>& callback) override;
    Return<sp<measurement_corrections::V1_0::IMeasurementCorrections>>
    getExtensionMeasurementCorrections() override;

   private:
    static sp<V2_0::IGnssCallback> sGnssCallback_2_0;
    static sp<V1_1::IGnssCallback> sGnssCallback_1_1;
};

}  // namespace implementation
}  // namespace V2_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_GNSS_V2_0_GNSS_H
