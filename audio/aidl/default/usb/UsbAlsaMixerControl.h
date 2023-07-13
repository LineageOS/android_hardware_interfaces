/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include <android-base/thread_annotations.h>
#include <android/binder_auto_utils.h>

#include "alsa/Mixer.h"

namespace aidl::android::hardware::audio::core::usb {

class UsbAlsaMixerControl {
  public:
    static UsbAlsaMixerControl& getInstance();

    void setDeviceConnectionState(int card, bool masterMuted, float masterVolume, bool connected);

    // Master volume settings will be applied to all sound cards, it is only set by the
    // USB module.
    ndk::ScopedAStatus setMasterMute(bool muted);
    ndk::ScopedAStatus setMasterVolume(float volume);
    // The volume settings can be different on sound cards. It is controlled by streams.
    ndk::ScopedAStatus setVolumes(int card, const std::vector<float>& volumes);

  private:
    std::shared_ptr<alsa::Mixer> getAlsaMixer(int card);
    std::map<int, std::shared_ptr<alsa::Mixer>> getAlsaMixers();

    std::mutex mLock;
    // A map whose key is the card number and value is a shared pointer to corresponding
    // AlsaMixer object.
    std::map<int, std::shared_ptr<alsa::Mixer>> mMixerControls GUARDED_BY(mLock);
};

}  // namespace aidl::android::hardware::audio::core::usb
