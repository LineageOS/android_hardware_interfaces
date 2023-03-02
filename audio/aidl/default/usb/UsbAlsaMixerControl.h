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
#include <optional>
#include <string>
#include <vector>

#include <android-base/thread_annotations.h>
#include <android/binder_auto_utils.h>

extern "C" {
#include <tinyalsa/mixer.h>
}

namespace aidl::android::hardware::audio::core::usb {

class MixerControl {
  public:
    explicit MixerControl(struct mixer_ctl* ctl);

    unsigned int getNumValues() const;
    int getMaxValue() const;
    int getMinValue() const;
    int setArray(const void* array, size_t count);

  private:
    std::mutex mLock;
    // The mixer_ctl object is owned by ALSA and will be released when the mixer is closed.
    struct mixer_ctl* mCtl GUARDED_BY(mLock);
    const unsigned int mNumValues;
    const int mMinValue;
    const int mMaxValue;
};

class AlsaMixer {
  public:
    explicit AlsaMixer(struct mixer* mixer);

    ~AlsaMixer();

    bool isValid() const { return mMixer != nullptr; }

    ndk::ScopedAStatus setMasterMute(bool muted);
    ndk::ScopedAStatus setMasterVolume(float volume);
    ndk::ScopedAStatus setVolumes(std::vector<float> volumes);

  private:
    enum Control {
        MASTER_SWITCH,
        MASTER_VOLUME,
        HW_VOLUME,
    };
    using ControlNamesAndExpectedCtlType = std::pair<std::string, enum mixer_ctl_type>;
    static const std::map<Control, std::vector<ControlNamesAndExpectedCtlType>> kPossibleControls;
    static std::map<Control, std::shared_ptr<MixerControl>> initializeMixerControls(
            struct mixer* mixer);

    // The mixer object is owned by ALSA and will be released when the mixer is closed.
    struct mixer* mMixer;
    // `mMixerControls` will only be initialized in constructor. After that, it wil only be
    // read but not be modified.
    const std::map<Control, std::shared_ptr<MixerControl>> mMixerControls;
};

class UsbAlsaMixerControl {
  public:
    static UsbAlsaMixerControl& getInstance();

    void setDeviceConnectionState(int card, bool masterMuted, float masterVolume, bool connected);

    // Master volume settings will be applied to all sound cards, it is only set by the
    // USB module.
    ndk::ScopedAStatus setMasterMute(bool muted);
    ndk::ScopedAStatus setMasterVolume(float volume);
    // The volume settings can be different on sound cards. It is controlled by streams.
    ndk::ScopedAStatus setVolumes(int card, std::vector<float> volumes);

  private:
    std::shared_ptr<AlsaMixer> getAlsaMixer(int card);
    std::map<int, std::shared_ptr<AlsaMixer>> getAlsaMixers();

    std::mutex mLock;
    // A map whose key is the card number and value is a shared pointer to corresponding
    // AlsaMixer object.
    std::map<int, std::shared_ptr<AlsaMixer>> mMixerControls GUARDED_BY(mLock);
};

}  // namespace aidl::android::hardware::audio::core::usb
