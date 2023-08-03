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

#include <algorithm>
#include <cmath>

#define LOG_TAG "AHAL_AlsaMixer"
#include <android-base/logging.h>
#include <android/binder_status.h>

#include "Mixer.h"

namespace aidl::android::hardware::audio::core::alsa {

// static
const std::map<Mixer::Control, std::vector<Mixer::ControlNamesAndExpectedCtlType>>
        Mixer::kPossibleControls = {
                {Mixer::MASTER_SWITCH, {{"Master Playback Switch", MIXER_CTL_TYPE_BOOL}}},
                {Mixer::MASTER_VOLUME, {{"Master Playback Volume", MIXER_CTL_TYPE_INT}}},
                {Mixer::HW_VOLUME,
                 {{"Headphone Playback Volume", MIXER_CTL_TYPE_INT},
                  {"Headset Playback Volume", MIXER_CTL_TYPE_INT},
                  {"PCM Playback Volume", MIXER_CTL_TYPE_INT}}},
                {Mixer::MIC_SWITCH, {{"Capture Switch", MIXER_CTL_TYPE_BOOL}}},
                {Mixer::MIC_GAIN, {{"Capture Volume", MIXER_CTL_TYPE_INT}}}};

// static
Mixer::Controls Mixer::initializeMixerControls(struct mixer* mixer) {
    if (mixer == nullptr) return {};
    Controls mixerControls;
    std::string mixerCtlNames;
    for (const auto& [control, possibleCtls] : kPossibleControls) {
        for (const auto& [ctlName, expectedCtlType] : possibleCtls) {
            struct mixer_ctl* ctl = mixer_get_ctl_by_name(mixer, ctlName.c_str());
            if (ctl != nullptr && mixer_ctl_get_type(ctl) == expectedCtlType) {
                mixerControls.emplace(control, ctl);
                if (!mixerCtlNames.empty()) {
                    mixerCtlNames += ",";
                }
                mixerCtlNames += ctlName;
                break;
            }
        }
    }
    LOG(DEBUG) << __func__ << ": available mixer control names=[" << mixerCtlNames << "]";
    return mixerControls;
}

std::ostream& operator<<(std::ostream& s, Mixer::Control c) {
    switch (c) {
        case Mixer::Control::MASTER_SWITCH:
            s << "master mute";
            break;
        case Mixer::Control::MASTER_VOLUME:
            s << "master volume";
            break;
        case Mixer::Control::HW_VOLUME:
            s << "volume";
            break;
        case Mixer::Control::MIC_SWITCH:
            s << "mic mute";
            break;
        case Mixer::Control::MIC_GAIN:
            s << "mic gain";
            break;
    }
    return s;
}

Mixer::Mixer(int card) : mMixer(mixer_open(card)), mMixerControls(initializeMixerControls(mMixer)) {
    if (!isValid()) {
        PLOG(ERROR) << __func__ << ": failed to open mixer for card=" << card;
    }
}

Mixer::~Mixer() {
    if (isValid()) {
        std::lock_guard l(mMixerAccess);
        mixer_close(mMixer);
    }
}

ndk::ScopedAStatus Mixer::setMasterMute(bool muted) {
    return setMixerControlMute(MASTER_SWITCH, muted);
}

ndk::ScopedAStatus Mixer::setMasterVolume(float volume) {
    return setMixerControlVolume(MASTER_VOLUME, volume);
}

ndk::ScopedAStatus Mixer::setMicGain(float gain) {
    return setMixerControlVolume(MIC_GAIN, gain);
}

ndk::ScopedAStatus Mixer::setMicMute(bool muted) {
    return setMixerControlMute(MIC_SWITCH, muted);
}

ndk::ScopedAStatus Mixer::setVolumes(const std::vector<float>& volumes) {
    if (!isValid()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    auto it = mMixerControls.find(Mixer::HW_VOLUME);
    if (it == mMixerControls.end()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    std::vector<int> percents;
    std::transform(
            volumes.begin(), volumes.end(), std::back_inserter(percents),
            [](float volume) -> int { return std::floor(std::clamp(volume, 0.0f, 1.0f) * 100); });
    std::lock_guard l(mMixerAccess);
    if (int err = setMixerControlPercent(it->second, percents); err != 0) {
        LOG(ERROR) << __func__ << ": failed to set volume, err=" << err;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Mixer::setMixerControlMute(Mixer::Control ctl, bool muted) {
    if (!isValid()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    auto it = mMixerControls.find(ctl);
    if (it == mMixerControls.end()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    std::lock_guard l(mMixerAccess);
    if (int err = setMixerControlValue(it->second, muted ? 0 : 1); err != 0) {
        LOG(ERROR) << __func__ << ": failed to set " << ctl << " to " << muted << ", err=" << err;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Mixer::setMixerControlVolume(Control ctl, float volume) {
    if (!isValid()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    auto it = mMixerControls.find(ctl);
    if (it == mMixerControls.end()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    volume = std::clamp(volume, 0.0f, 1.0f);
    std::lock_guard l(mMixerAccess);
    if (int err = setMixerControlPercent(it->second, std::floor(volume * 100)); err != 0) {
        LOG(ERROR) << __func__ << ": failed to set " << ctl << " to " << volume << ", err=" << err;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    return ndk::ScopedAStatus::ok();
}

int Mixer::setMixerControlPercent(struct mixer_ctl* ctl, int percent) {
    int ret = 0;
    const unsigned int n = mixer_ctl_get_num_values(ctl);
    for (unsigned int id = 0; id < n; id++) {
        if (int error = mixer_ctl_set_percent(ctl, id, percent); error != 0) {
            ret = error;
        }
    }
    return ret;
}

int Mixer::setMixerControlPercent(struct mixer_ctl* ctl, const std::vector<int>& percents) {
    int ret = 0;
    const unsigned int n = mixer_ctl_get_num_values(ctl);
    for (unsigned int id = 0; id < n; id++) {
        if (int error = mixer_ctl_set_percent(ctl, id, id < percents.size() ? percents[id] : 0);
            error != 0) {
            ret = error;
        }
    }
    return ret;
}

int Mixer::setMixerControlValue(struct mixer_ctl* ctl, int value) {
    int ret = 0;
    const unsigned int n = mixer_ctl_get_num_values(ctl);
    for (unsigned int id = 0; id < n; id++) {
        if (int error = mixer_ctl_set_value(ctl, id, value); error != 0) {
            ret = error;
        }
    }
    return ret;
}

}  // namespace aidl::android::hardware::audio::core::alsa
