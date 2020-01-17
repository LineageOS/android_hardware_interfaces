/*
 * Copyright (C) 2020 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_GNSS_V2_1_GNSSANTENNAINFO_H
#define ANDROID_HARDWARE_GNSS_V2_1_GNSSANTENNAINFO_H

#include <android/hardware/gnss/2.1/IGnssAntennaInfo.h>

#include <mutex>
#include <thread>

namespace android {
namespace hardware {
namespace gnss {
namespace V2_1 {
namespace implementation {

using ::android::sp;
using ::android::hardware::Return;
using ::android::hardware::Void;

struct GnssAntennaInfo : public IGnssAntennaInfo {
    GnssAntennaInfo();
    ~GnssAntennaInfo();

    // Methods from ::android::hardware::gnss::V2_1::IGnssAntennaInfo follow.
    Return<GnssAntennaInfoStatus> setCallback(
            const sp<IGnssAntennaInfoCallback>& callback) override;
    Return<void> close() override;

  private:
    void start();
    void stop();
    void reportAntennaInfo(
            const hidl_vec<IGnssAntennaInfoCallback::GnssAntennaInfo>& antennaInfo) const;

    static sp<IGnssAntennaInfoCallback> sCallback;
    std::atomic<long> mMinIntervalMillis;
    std::atomic<bool> mIsActive;
    std::thread mThread;
    mutable std::mutex mMutex;
};

}  // namespace implementation
}  // namespace V2_1
}  // namespace gnss
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_GNSS_V2_1_GNSSCONFIGURATION_H