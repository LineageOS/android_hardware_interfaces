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

#ifndef ANDROID_HARDWARE_GNSS_V2_0_GNSSMEASUREMENT_H
#define ANDROID_HARDWARE_GNSS_V2_0_GNSSMEASUREMENT_H

#include <android/hardware/gnss/2.0/IGnssMeasurement.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <atomic>
#include <mutex>
#include <thread>

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

using GnssData = V2_0::IGnssMeasurementCallback::GnssData;

struct GnssMeasurement : public IGnssMeasurement {
    GnssMeasurement();
    ~GnssMeasurement();
    // Methods from V1_0::IGnssMeasurement follow.
    Return<V1_0::IGnssMeasurement::GnssMeasurementStatus> setCallback(
        const sp<V1_0::IGnssMeasurementCallback>& callback) override;
    Return<void> close() override;

    // Methods from V1_1::IGnssMeasurement follow.
    Return<V1_0::IGnssMeasurement::GnssMeasurementStatus> setCallback_1_1(
        const sp<V1_1::IGnssMeasurementCallback>& callback, bool enableFullTracking) override;

    // Methods from V2_0::IGnssMeasurement follow.
    Return<V1_0::IGnssMeasurement::GnssMeasurementStatus> setCallback_2_0(
        const sp<V2_0::IGnssMeasurementCallback>& callback, bool enableFullTracking) override;

   private:
    void start();
    void stop();
    void reportMeasurement(const GnssData&);

    // Guarded by mMutex
    static sp<IGnssMeasurementCallback> sCallback;

    std::atomic<long> mMinIntervalMillis;
    std::atomic<bool> mIsActive;
    std::thread mThread;

    // Synchronization lock for sCallback
    mutable std::mutex mMutex;
};

}  // namespace implementation
}  // namespace V2_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_GNSS_V2_0_GNSSMEASUREMENT_H
