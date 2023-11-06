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

#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <android-base/thread_annotations.h>
#include <android/binder_auto_utils.h>

extern "C" {
#include <tinyalsa/mixer.h>
}

namespace aidl::android::hardware::audio::core::alsa {

class Mixer {
  public:
    explicit Mixer(int card);
    ~Mixer();

    bool isValid() const { return mMixer != nullptr; }

    ndk::ScopedAStatus getMasterMute(bool* muted);
    ndk::ScopedAStatus getMasterVolume(float* volume);
    ndk::ScopedAStatus getMicGain(float* gain);
    ndk::ScopedAStatus getMicMute(bool* muted);
    ndk::ScopedAStatus getVolumes(std::vector<float>* volumes);
    ndk::ScopedAStatus setMasterMute(bool muted);
    ndk::ScopedAStatus setMasterVolume(float volume);
    ndk::ScopedAStatus setMicGain(float gain);
    ndk::ScopedAStatus setMicMute(bool muted);
    ndk::ScopedAStatus setVolumes(const std::vector<float>& volumes);

  private:
    enum Control {
        MASTER_SWITCH,
        MASTER_VOLUME,
        HW_VOLUME,
        MIC_SWITCH,
        MIC_GAIN,
    };
    using ControlNamesAndExpectedCtlType = std::pair<std::string, enum mixer_ctl_type>;
    using Controls = std::map<Control, struct mixer_ctl*>;

    friend std::ostream& operator<<(std::ostream&, Control);
    static const std::map<Control, std::vector<ControlNamesAndExpectedCtlType>> kPossibleControls;
    static Controls initializeMixerControls(struct mixer* mixer);

    ndk::ScopedAStatus findControl(Control ctl, struct mixer_ctl** result);
    ndk::ScopedAStatus getMixerControlMute(Control ctl, bool* muted);
    ndk::ScopedAStatus getMixerControlVolume(Control ctl, float* volume);
    ndk::ScopedAStatus setMixerControlMute(Control ctl, bool muted);
    ndk::ScopedAStatus setMixerControlVolume(Control ctl, float volume);

    int getMixerControlPercent(struct mixer_ctl* ctl, std::vector<int>* percents)
            REQUIRES(mMixerAccess);
    int getMixerControlValues(struct mixer_ctl* ctl, std::vector<int>* values)
            REQUIRES(mMixerAccess);
    int setMixerControlPercent(struct mixer_ctl* ctl, int percent) REQUIRES(mMixerAccess);
    int setMixerControlPercent(struct mixer_ctl* ctl, const std::vector<int>& percents)
            REQUIRES(mMixerAccess);
    int setMixerControlValue(struct mixer_ctl* ctl, int value) REQUIRES(mMixerAccess);

    // Since ALSA functions do not use internal locking, enforce thread safety at our level.
    std::mutex mMixerAccess;
    // The mixer object is owned by ALSA and will be released when the mixer is closed.
    struct mixer* const mMixer;
    // `mMixerControls` will only be initialized in constructor. After that, it will only be
    // read but not be modified. Each mixer_ctl object is owned by ALSA, it's life span is
    // the same as of the mixer itself.
    const Controls mMixerControls;
};

}  // namespace aidl::android::hardware::audio::core::alsa
