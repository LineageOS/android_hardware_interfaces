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

#define LOG_TAG "AHAL_UsbAlsaMixerControl"
#include <android-base/logging.h>

#include <cmath>
#include <string>
#include <vector>

#include <android/binder_status.h>

#include "UsbAlsaMixerControl.h"

namespace aidl::android::hardware::audio::core::usb {

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
const std::map<AlsaMixer::Control, std::vector<AlsaMixer::ControlNamesAndExpectedCtlType>>
        AlsaMixer::kPossibleControls = {
                {AlsaMixer::MASTER_SWITCH, {{"Master Playback Switch", MIXER_CTL_TYPE_BOOL}}},
                {AlsaMixer::MASTER_VOLUME, {{"Master Playback Volume", MIXER_CTL_TYPE_INT}}},
                {AlsaMixer::HW_VOLUME,
                 {{"Headphone Playback Volume", MIXER_CTL_TYPE_INT},
                  {"Headset Playback Volume", MIXER_CTL_TYPE_INT},
                  {"PCM Playback Volume", MIXER_CTL_TYPE_INT}}}};

// static
std::map<AlsaMixer::Control, std::shared_ptr<MixerControl>> AlsaMixer::initializeMixerControls(
        struct mixer* mixer) {
    std::map<AlsaMixer::Control, std::shared_ptr<MixerControl>> mixerControls;
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

AlsaMixer::AlsaMixer(struct mixer* mixer)
    : mMixer(mixer), mMixerControls(initializeMixerControls(mMixer)) {}

AlsaMixer::~AlsaMixer() {
    mixer_close(mMixer);
}

namespace {

int volumeFloatToInteger(float fValue, int maxValue, int minValue) {
    return minValue + std::ceil((maxValue - minValue) * fValue);
}

}  // namespace

ndk::ScopedAStatus AlsaMixer::setMasterMute(bool muted) {
    auto it = mMixerControls.find(AlsaMixer::MASTER_SWITCH);
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

ndk::ScopedAStatus AlsaMixer::setMasterVolume(float volume) {
    auto it = mMixerControls.find(AlsaMixer::MASTER_VOLUME);
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

ndk::ScopedAStatus AlsaMixer::setVolumes(std::vector<float> volumes) {
    auto it = mMixerControls.find(AlsaMixer::HW_VOLUME);
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

//-----------------------------------------------------------------------------

// static
UsbAlsaMixerControl& UsbAlsaMixerControl::getInstance() {
    static UsbAlsaMixerControl gInstance;
    return gInstance;
}

void UsbAlsaMixerControl::setDeviceConnectionState(int card, bool masterMuted, float masterVolume,
                                                   bool connected) {
    LOG(DEBUG) << __func__ << ": card=" << card << ", connected=" << connected;
    if (connected) {
        struct mixer* mixer = mixer_open(card);
        if (mixer == nullptr) {
            PLOG(ERROR) << __func__ << ": failed to open mixer for card=" << card;
            return;
        }
        auto alsaMixer = std::make_shared<AlsaMixer>(mixer);
        alsaMixer->setMasterMute(masterMuted);
        alsaMixer->setMasterVolume(masterVolume);
        const std::lock_guard guard(mLock);
        mMixerControls.emplace(card, alsaMixer);
    } else {
        const std::lock_guard guard(mLock);
        mMixerControls.erase(card);
    }
}

ndk::ScopedAStatus UsbAlsaMixerControl::setMasterMute(bool mute) {
    auto alsaMixers = getAlsaMixers();
    for (auto it = alsaMixers.begin(); it != alsaMixers.end(); ++it) {
        if (auto result = it->second->setMasterMute(mute); !result.isOk()) {
            // Return illegal state if there are multiple devices connected and one of them fails
            // to set master mute. Otherwise, return the error from calling `setMasterMute`.
            LOG(ERROR) << __func__ << ": failed to set master mute for card=" << it->first;
            return alsaMixers.size() > 1 ? ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE)
                                         : std::move(result);
        }
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus UsbAlsaMixerControl::setMasterVolume(float volume) {
    auto alsaMixers = getAlsaMixers();
    for (auto it = alsaMixers.begin(); it != alsaMixers.end(); ++it) {
        if (auto result = it->second->setMasterVolume(volume); !result.isOk()) {
            // Return illegal state if there are multiple devices connected and one of them fails
            // to set master volume. Otherwise, return the error from calling `setMasterVolume`.
            LOG(ERROR) << __func__ << ": failed to set master volume for card=" << it->first;
            return alsaMixers.size() > 1 ? ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE)
                                         : std::move(result);
        }
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus UsbAlsaMixerControl::setVolumes(int card, std::vector<float> volumes) {
    auto alsaMixer = getAlsaMixer(card);
    if (alsaMixer == nullptr) {
        LOG(ERROR) << __func__ << ": no mixer control found for card=" << card;
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    return alsaMixer->setVolumes(volumes);
}

std::shared_ptr<AlsaMixer> UsbAlsaMixerControl::getAlsaMixer(int card) {
    const std::lock_guard guard(mLock);
    const auto it = mMixerControls.find(card);
    return it == mMixerControls.end() ? nullptr : it->second;
}

std::map<int, std::shared_ptr<AlsaMixer>> UsbAlsaMixerControl::getAlsaMixers() {
    const std::lock_guard guard(mLock);
    return mMixerControls;
}

}  // namespace aidl::android::hardware::audio::core::usb
