/*
 * Copyright 2021 The Android Open Source Project
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

#include <aidl/android/hardware/tv/tuner/BnTimeFilter.h>
#include "Demux.h"
#include "time.h"

using namespace std;

namespace aidl {
namespace android {
namespace hardware {
namespace tv {
namespace tuner {

#define INVALID_TIME_STAMP -1

class Demux;

class TimeFilter : public BnTimeFilter {
  public:
    TimeFilter();
    TimeFilter(std::shared_ptr<Demux> demux);
    ~TimeFilter();

    ::ndk::ScopedAStatus setTimeStamp(int64_t in_timeStamp) override;
    ::ndk::ScopedAStatus clearTimeStamp() override;
    ::ndk::ScopedAStatus getTimeStamp(int64_t* _aidl_return) override;
    ::ndk::ScopedAStatus getSourceTime(int64_t* _aidl_return) override;
    ::ndk::ScopedAStatus close() override;

    binder_status_t dump(int fd, const char** args, uint32_t numArgs) override;

  private:
    ::std::shared_ptr<Demux> mDemux;
    uint64_t mTimeStamp = INVALID_TIME_STAMP;
    time_t mBeginTime;
};

}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
}  // namespace aidl
