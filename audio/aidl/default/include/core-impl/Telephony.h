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

#include <android/binder_enums.h>

#include <aidl/android/hardware/audio/core/BnTelephony.h>

namespace aidl::android::hardware::audio::core {

class Telephony : public BnTelephony {
  public:
    Telephony();

  private:
    ndk::ScopedAStatus getSupportedAudioModes(std::vector<AudioMode>* _aidl_return) override;
    ndk::ScopedAStatus switchAudioMode(AudioMode in_mode) override;
    ndk::ScopedAStatus setTelecomConfig(const TelecomConfig& in_config,
                                        TelecomConfig* _aidl_return) override;

    const std::vector<AudioMode> mSupportedAudioModes = {::ndk::enum_range<AudioMode>().begin(),
                                                         ::ndk::enum_range<AudioMode>().end()};
    TelecomConfig mTelecomConfig;
};

}  // namespace aidl::android::hardware::audio::core
