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

#include <aidl/android/hardware/audio/core/sounddose/ISoundDose.h>
#include <aidl/android/hardware/audio/sounddose/BnSoundDoseFactory.h>
#include <android/binder_interface_utils.h>

#include <unordered_map>

namespace aidl::android::hardware::audio::sounddose {

using ::aidl::android::hardware::audio::core::sounddose::ISoundDose;

class SoundDoseFactory : public BnSoundDoseFactory {
  public:
    ndk::ScopedAStatus getSoundDose(const std::string& module,
                                    std::shared_ptr<ISoundDose>* _aidl_return) override;

  private:
    std::unordered_map<std::string, ndk::SpAIBinder> mSoundDoseBinderMap;
};

}  // namespace aidl::android::hardware::audio::sounddose
