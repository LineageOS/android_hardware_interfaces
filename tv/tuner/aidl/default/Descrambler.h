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

#include <aidl/android/hardware/tv/tuner/BnDescrambler.h>
#include <aidl/android/hardware/tv/tuner/ITuner.h>
#include <inttypes.h>

using namespace std;

namespace aidl {
namespace android {
namespace hardware {
namespace tv {
namespace tuner {

class Descrambler : public BnDescrambler {
  public:
    Descrambler();

    ::ndk::ScopedAStatus setDemuxSource(int32_t in_demuxId) override;
    ::ndk::ScopedAStatus setKeyToken(const std::vector<uint8_t>& in_keyToken) override;
    ::ndk::ScopedAStatus addPid(const DemuxPid& in_pid,
                                const std::shared_ptr<IFilter>& in_optionalSourceFilter) override;
    ::ndk::ScopedAStatus removePid(
            const DemuxPid& in_pid,
            const std::shared_ptr<IFilter>& in_optionalSourceFilter) override;
    ::ndk::ScopedAStatus close() override;

  private:
    virtual ~Descrambler();
    int32_t mSourceDemuxId;
    bool mDemuxSet = false;
};

}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
}  // namespace aidl
