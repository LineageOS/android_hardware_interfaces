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

#include <aidl/android/hardware/tv/tuner/BnLnb.h>
#include <aidl/android/hardware/tv/tuner/ITuner.h>

using namespace std;

namespace aidl {
namespace android {
namespace hardware {
namespace tv {
namespace tuner {

class Lnb : public BnLnb {
  public:
    Lnb();
    Lnb(int id);

    ::ndk::ScopedAStatus setCallback(const std::shared_ptr<ILnbCallback>& in_callback) override;
    ::ndk::ScopedAStatus setVoltage(LnbVoltage in_voltage) override;
    ::ndk::ScopedAStatus setTone(LnbTone in_tone) override;
    ::ndk::ScopedAStatus setSatellitePosition(LnbPosition in_position) override;
    ::ndk::ScopedAStatus sendDiseqcMessage(const std::vector<uint8_t>& in_diseqcMessage) override;
    ::ndk::ScopedAStatus close() override;

    int getId();

  private:
    int mId;
    virtual ~Lnb();
    std::shared_ptr<ILnbCallback> mCallback;
};

}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
}  // namespace aidl
