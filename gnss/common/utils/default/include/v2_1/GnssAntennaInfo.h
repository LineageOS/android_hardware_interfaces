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

#pragma once

#include <android/hardware/gnss/2.1/IGnssAntennaInfo.h>

#include <mutex>
#include <thread>

namespace android::hardware::gnss::V2_1::implementation {

struct GnssAntennaInfo : public ::android::hardware::gnss::V2_1::IGnssAntennaInfo {
    GnssAntennaInfo();
    ~GnssAntennaInfo();

    // Methods from ::android::hardware::gnss::V2_1::IGnssAntennaInfo follow.
    Return<GnssAntennaInfoStatus> setCallback(
            const sp<::android::hardware::gnss::V2_1::IGnssAntennaInfoCallback>& callback) override;
    Return<void> close() override;

  private:
    void start();
    void stop();
    void reportAntennaInfo(
            const hidl_vec<
                    ::android::hardware::gnss::V2_1::IGnssAntennaInfoCallback::GnssAntennaInfo>&
                    antennaInfo) const;

    // Guarded by mMutex
    static sp<::android::hardware::gnss::V2_1::IGnssAntennaInfoCallback> sCallback;

    std::atomic<long> mMinIntervalMillis;
    std::atomic<bool> mIsActive;
    std::thread mThread;

    // Synchronization lock for sCallback
    mutable std::mutex mMutex;
};

}  // namespace android::hardware::gnss::V2_1::implementation
