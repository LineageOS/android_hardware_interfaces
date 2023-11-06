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
#include <error/expected_utils.h>

#include "Mixer.h"

namespace ndk {

// This enables use of 'error/expected_utils' for ScopedAStatus.

inline bool errorIsOk(const ScopedAStatus& s) {
    return s.isOk();
}

inline std::string errorToString(const ScopedAStatus& s) {
    return s.getDescription();
}

}  // namespace ndk

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

ndk::ScopedAStatus Mixer::getMasterMute(bool* muted) {
    return getMixerControlMute(MASTER_SWITCH, muted);
}

ndk::ScopedAStatus Mixer::getMasterVolume(float* volume) {
    return getMixerControlVolume(MASTER_VOLUME, volume);
}

ndk::ScopedAStatus Mixer::getMicGain(float* gain) {
    return getMixerControlVolume(MIC_GAIN, gain);
}

ndk::ScopedAStatus Mixer::getMicMute(bool* muted) {
    return getMixerControlMute(MIC_SWITCH, muted);
}

ndk::ScopedAStatus Mixer::getVolumes(std::vector<float>* volumes) {
    struct mixer_ctl* mctl;
    RETURN_STATUS_IF_ERROR(findControl(Mixer::HW_VOLUME, &mctl));
    std::vector<int> percents;
    std::lock_guard l(mMixerAccess);
    if (int err = getMixerControlPercent(mctl, &percents); err != 0) {
        LOG(ERROR) << __func__ << ": failed to get volume, err=" << err;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    std::transform(percents.begin(), percents.end(), std::back_inserter(*volumes),
                   [](int percent) -> float { return std::clamp(percent / 100.0f, 0.0f, 1.0f); });
    return ndk::ScopedAStatus::ok();
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
    struct mixer_ctl* mctl;
    RETURN_STATUS_IF_ERROR(findControl(Mixer::HW_VOLUME, &mctl));
    std::vector<int> percents;
    std::transform(
            volumes.begin(), volumes.end(), std::back_inserter(percents),
            [](float volume) -> int { return std::floor(std::clamp(volume, 0.0f, 1.0f) * 100); });
    std::lock_guard l(mMixerAccess);
    if (int err = setMixerControlPercent(mctl, percents); err != 0) {
        LOG(ERROR) << __func__ << ": failed to set volume, err=" << err;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Mixer::findControl(Control ctl, struct mixer_ctl** result) {
    if (!isValid()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    if (auto it = mMixerControls.find(ctl); it != mMixerControls.end()) {
        *result = it->second;
        return ndk::ScopedAStatus::ok();
    }
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Mixer::getMixerControlMute(Control ctl, bool* muted) {
    struct mixer_ctl* mctl;
    RETURN_STATUS_IF_ERROR(findControl(ctl, &mctl));
    std::lock_guard l(mMixerAccess);
    std::vector<int> mutedValues;
    if (int err = getMixerControlValues(mctl, &mutedValues); err != 0) {
        LOG(ERROR) << __func__ << ": failed to get " << ctl << ", err=" << err;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    if (mutedValues.empty()) {
        LOG(ERROR) << __func__ << ": got no values for " << ctl;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    *muted = mutedValues[0] != 0;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Mixer::getMixerControlVolume(Control ctl, float* volume) {
    struct mixer_ctl* mctl;
    RETURN_STATUS_IF_ERROR(findControl(ctl, &mctl));
    std::lock_guard l(mMixerAccess);
    std::vector<int> percents;
    if (int err = getMixerControlPercent(mctl, &percents); err != 0) {
        LOG(ERROR) << __func__ << ": failed to get " << ctl << ", err=" << err;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    if (percents.empty()) {
        LOG(ERROR) << __func__ << ": got no values for " << ctl;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    *volume = std::clamp(percents[0] / 100.0f, 0.0f, 1.0f);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Mixer::setMixerControlMute(Control ctl, bool muted) {
    struct mixer_ctl* mctl;
    RETURN_STATUS_IF_ERROR(findControl(ctl, &mctl));
    std::lock_guard l(mMixerAccess);
    if (int err = setMixerControlValue(mctl, muted ? 0 : 1); err != 0) {
        LOG(ERROR) << __func__ << ": failed to set " << ctl << " to " << muted << ", err=" << err;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Mixer::setMixerControlVolume(Control ctl, float volume) {
    struct mixer_ctl* mctl;
    RETURN_STATUS_IF_ERROR(findControl(ctl, &mctl));
    volume = std::clamp(volume, 0.0f, 1.0f);
    std::lock_guard l(mMixerAccess);
    if (int err = setMixerControlPercent(mctl, std::floor(volume * 100)); err != 0) {
        LOG(ERROR) << __func__ << ": failed to set " << ctl << " to " << volume << ", err=" << err;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    return ndk::ScopedAStatus::ok();
}

int Mixer::getMixerControlPercent(struct mixer_ctl* ctl, std::vector<int>* percents) {
    const unsigned int n = mixer_ctl_get_num_values(ctl);
    percents->resize(n);
    for (unsigned int id = 0; id < n; id++) {
        if (int valueOrError = mixer_ctl_get_percent(ctl, id); valueOrError >= 0) {
            (*percents)[id] = valueOrError;
        } else {
            return valueOrError;
        }
    }
    return 0;
}

int Mixer::getMixerControlValues(struct mixer_ctl* ctl, std::vector<int>* values) {
    const unsigned int n = mixer_ctl_get_num_values(ctl);
    values->resize(n);
    for (unsigned int id = 0; id < n; id++) {
        if (int valueOrError = mixer_ctl_get_value(ctl, id); valueOrError >= 0) {
            (*values)[id] = valueOrError;
        } else {
            return valueOrError;
        }
    }
    return 0;
}

int Mixer::setMixerControlPercent(struct mixer_ctl* ctl, int percent) {
    const unsigned int n = mixer_ctl_get_num_values(ctl);
    for (unsigned int id = 0; id < n; id++) {
        if (int error = mixer_ctl_set_percent(ctl, id, percent); error != 0) {
            return error;
        }
    }
    return 0;
}

int Mixer::setMixerControlPercent(struct mixer_ctl* ctl, const std::vector<int>& percents) {
    const unsigned int n = mixer_ctl_get_num_values(ctl);
    for (unsigned int id = 0; id < n; id++) {
        if (int error = mixer_ctl_set_percent(ctl, id, id < percents.size() ? percents[id] : 0);
            error != 0) {
            return error;
        }
    }
    return 0;
}

int Mixer::setMixerControlValue(struct mixer_ctl* ctl, int value) {
    const unsigned int n = mixer_ctl_get_num_values(ctl);
    for (unsigned int id = 0; id < n; id++) {
        if (int error = mixer_ctl_set_value(ctl, id, value); error != 0) {
            return error;
        }
    }
    return 0;
}

}  // namespace aidl::android::hardware::audio::core::alsa
