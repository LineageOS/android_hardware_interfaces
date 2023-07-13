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

#include <android/binder_status.h>

#include "UsbAlsaMixerControl.h"

namespace aidl::android::hardware::audio::core::usb {

// static
UsbAlsaMixerControl& UsbAlsaMixerControl::getInstance() {
    static UsbAlsaMixerControl gInstance;
    return gInstance;
}

void UsbAlsaMixerControl::setDeviceConnectionState(int card, bool masterMuted, float masterVolume,
                                                   bool connected) {
    LOG(DEBUG) << __func__ << ": card=" << card << ", connected=" << connected;
    if (connected) {
        auto alsaMixer = std::make_shared<alsa::Mixer>(card);
        if (!alsaMixer->isValid()) {
            return;
        }
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

ndk::ScopedAStatus UsbAlsaMixerControl::setVolumes(int card, const std::vector<float>& volumes) {
    auto alsaMixer = getAlsaMixer(card);
    if (alsaMixer == nullptr) {
        LOG(ERROR) << __func__ << ": no mixer control found for card=" << card;
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    return alsaMixer->setVolumes(volumes);
}

std::shared_ptr<alsa::Mixer> UsbAlsaMixerControl::getAlsaMixer(int card) {
    const std::lock_guard guard(mLock);
    const auto it = mMixerControls.find(card);
    return it == mMixerControls.end() ? nullptr : it->second;
}

std::map<int, std::shared_ptr<alsa::Mixer>> UsbAlsaMixerControl::getAlsaMixers() {
    const std::lock_guard guard(mLock);
    return mMixerControls;
}

}  // namespace aidl::android::hardware::audio::core::usb
