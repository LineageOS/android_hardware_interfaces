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

#ifndef ANDROID_HARDWARE_TV_TUNER_V1_0_TIMEFILTER_H_
#define ANDROID_HARDWARE_TV_TUNER_V1_0_TIMEFILTER_H_

#include <android/hardware/tv/tuner/1.0/ITimeFilter.h>
#include "Demux.h"
#include "time.h"

using namespace std;

namespace android {
namespace hardware {
namespace tv {
namespace tuner {
namespace V1_0 {
namespace implementation {

using ::android::hardware::tv::tuner::V1_0::IDemux;
using ::android::hardware::tv::tuner::V1_0::IFilterCallback;
using ::android::hardware::tv::tuner::V1_0::Result;

using FilterMQ = MessageQueue<uint8_t, kSynchronizedReadWrite>;

#define INVALID_TIME_STAMP -1

class Demux;

class TimeFilter : public ITimeFilter {
  public:
    TimeFilter();

    TimeFilter(sp<Demux> demux);

    ~TimeFilter();

    virtual Return<Result> setTimeStamp(uint64_t timeStamp) override;

    virtual Return<Result> clearTimeStamp() override;

    virtual Return<void> getTimeStamp(getTimeStamp_cb _hidl_cb) override;

    virtual Return<void> getSourceTime(getSourceTime_cb _hidl_cb) override;

    virtual Return<Result> close() override;

  private:
    sp<Demux> mDemux;
    uint64_t mTimeStamp = INVALID_TIME_STAMP;
    time_t mBeginTime;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_TV_TUNER_V1_0_TIMEFILTER_H_