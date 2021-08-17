/*
 * Copyright 2020 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_TV_TUNER_V1_1_DESCRAMBLER_H_
#define ANDROID_HARDWARE_TV_TUNER_V1_1_DESCRAMBLER_H_

#include <android/hardware/tv/tuner/1.0/IDescrambler.h>
#include <android/hardware/tv/tuner/1.1/ITuner.h>
#include <inttypes.h>

using namespace std;

namespace android {
namespace hardware {
namespace tv {
namespace tuner {
namespace V1_0 {
namespace implementation {

class Descrambler : public IDescrambler {
  public:
    Descrambler();

    virtual Return<Result> setDemuxSource(uint32_t demuxId) override;

    virtual Return<Result> setKeyToken(const hidl_vec<uint8_t>& keyToken) override;

    virtual Return<Result> addPid(const DemuxPid& pid,
                                  const sp<IFilter>& optionalSourceFilter) override;

    virtual Return<Result> removePid(const DemuxPid& pid,
                                     const sp<IFilter>& optionalSourceFilter) override;

    virtual Return<Result> close() override;

  private:
    virtual ~Descrambler();
    uint32_t mSourceDemuxId;
    bool mDemuxSet = false;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_TV_TUNER_V1_DESCRAMBLER_H_
