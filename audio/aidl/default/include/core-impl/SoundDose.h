/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <mutex>

#include <aidl/android/hardware/audio/core/sounddose/BnSoundDose.h>
#include <aidl/android/media/audio/common/AudioDevice.h>

using aidl::android::media::audio::common::AudioDevice;

namespace aidl::android::hardware::audio::core::sounddose {

class SoundDose : public BnSoundDose {
  public:
    SoundDose() : mRs2Value(DEFAULT_MAX_RS2){};

    ndk::ScopedAStatus setOutputRs2(float in_rs2ValueDbA) override;
    ndk::ScopedAStatus getOutputRs2(float* _aidl_return) override;
    ndk::ScopedAStatus registerSoundDoseCallback(
            const std::shared_ptr<ISoundDose::IHalSoundDoseCallback>& in_callback) override;

  private:
    std::shared_ptr<ISoundDose::IHalSoundDoseCallback> mCallback;
    float mRs2Value;
};

}  // namespace aidl::android::hardware::audio::core::sounddose
