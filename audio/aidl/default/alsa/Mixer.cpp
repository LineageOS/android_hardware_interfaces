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

#define LOG_TAG "AHAL_AlsaMixer"
#include <android-base/logging.h>

#include <cmath>

#include <android/binder_status.h>

#include "Mixer.h"

namespace aidl::android::hardware::audio::core::alsa {

//-----------------------------------------------------------------------------

MixerControl::MixerControl(struct mixer_ctl* ctl)
    : mCtl(ctl),
      mNumValues(mixer_ctl_get_num_values(ctl)),
      mMinValue(mixer_ctl_get_range_min(ctl)),
      mMaxValue(mixer_ctl_get_range_max(ctl)) {}

unsigned int MixerControl::getNumValues() const {
    return mNumValues;
}

int MixerControl::getMaxValue() const {
    return mMaxValue;
}

int MixerControl::getMinValue() const {
    return mMinValue;
}

int MixerControl::setArray(const void* array, size_t count) {
    const std::lock_guard guard(mLock);
    return mixer_ctl_set_array(mCtl, array, count);
}

//-----------------------------------------------------------------------------

// static
const std::map<Mixer::Control, std::vector<Mixer::ControlNamesAndExpectedCtlType>>
        Mixer::kPossibleControls = {
                {Mixer::MASTER_SWITCH, {{"Master Playback Switch", MIXER_CTL_TYPE_BOOL}}},
                {Mixer::MASTER_VOLUME, {{"Master Playback Volume", MIXER_CTL_TYPE_INT}}},
                {Mixer::HW_VOLUME,
                 {{"Headphone Playback Volume", MIXER_CTL_TYPE_INT},
                  {"Headset Playback Volume", MIXER_CTL_TYPE_INT},
                  {"PCM Playback Volume", MIXER_CTL_TYPE_INT}}}};

// static
std::map<Mixer::Control, std::shared_ptr<MixerControl>> Mixer::initializeMixerControls(
        struct mixer* mixer) {
    std::map<Mixer::Control, std::shared_ptr<MixerControl>> mixerControls;
    std::string mixerCtlNames;
    for (const auto& [control, possibleCtls] : kPossibleControls) {
        for (const auto& [ctlName, expectedCtlType] : possibleCtls) {
            struct mixer_ctl* ctl = mixer_get_ctl_by_name(mixer, ctlName.c_str());
            if (ctl != nullptr && mixer_ctl_get_type(ctl) == expectedCtlType) {
                mixerControls.emplace(control, std::make_unique<MixerControl>(ctl));
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

Mixer::Mixer(struct mixer* mixer)
    : mMixer(mixer), mMixerControls(initializeMixerControls(mMixer)) {}

Mixer::~Mixer() {
    mixer_close(mMixer);
}

namespace {

int volumeFloatToInteger(float fValue, int maxValue, int minValue) {
    return minValue + std::ceil((maxValue - minValue) * fValue);
}

}  // namespace

ndk::ScopedAStatus Mixer::setMasterMute(bool muted) {
    auto it = mMixerControls.find(Mixer::MASTER_SWITCH);
    if (it == mMixerControls.end()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    const int numValues = it->second->getNumValues();
    std::vector<int> values(numValues, muted ? 0 : 1);
    if (int err = it->second->setArray(values.data(), numValues); err != 0) {
        LOG(ERROR) << __func__ << ": failed to set master mute, err=" << err;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Mixer::setMasterVolume(float volume) {
    auto it = mMixerControls.find(Mixer::MASTER_VOLUME);
    if (it == mMixerControls.end()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    const int numValues = it->second->getNumValues();
    std::vector<int> values(numValues, volumeFloatToInteger(volume, it->second->getMaxValue(),
                                                            it->second->getMinValue()));
    if (int err = it->second->setArray(values.data(), numValues); err != 0) {
        LOG(ERROR) << __func__ << ": failed to set master volume, err=" << err;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Mixer::setVolumes(const std::vector<float>& volumes) {
    auto it = mMixerControls.find(Mixer::HW_VOLUME);
    if (it == mMixerControls.end()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    const int numValues = it->second->getNumValues();
    if (numValues < 0) {
        LOG(FATAL) << __func__ << ": negative number of values: " << numValues;
    }
    const int maxValue = it->second->getMaxValue();
    const int minValue = it->second->getMinValue();
    std::vector<int> values;
    size_t i = 0;
    for (; i < static_cast<size_t>(numValues) && i < values.size(); ++i) {
        values.emplace_back(volumeFloatToInteger(volumes[i], maxValue, minValue));
    }
    if (int err = it->second->setArray(values.data(), values.size()); err != 0) {
        LOG(ERROR) << __func__ << ": failed to set volume, err=" << err;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::audio::core::alsa
