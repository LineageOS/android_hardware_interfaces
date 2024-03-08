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

#define LOG_TAG "AHAL_ModuleAlsa"

#include <vector>

#include <android-base/logging.h>

#include "Utils.h"
#include "core-impl/ModuleAlsa.h"

extern "C" {
#include "alsa_device_profile.h"
}

using aidl::android::media::audio::common::AudioChannelLayout;
using aidl::android::media::audio::common::AudioFormatType;
using aidl::android::media::audio::common::AudioPort;
using aidl::android::media::audio::common::AudioProfile;

namespace aidl::android::hardware::audio::core {

ndk::ScopedAStatus ModuleAlsa::populateConnectedDevicePort(AudioPort* audioPort) {
    auto deviceProfile = alsa::getDeviceProfile(*audioPort);
    if (!deviceProfile.has_value()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    auto proxy = alsa::readAlsaDeviceInfo(*deviceProfile);
    if (proxy.get() == nullptr) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    alsa_device_profile* profile = proxy.getProfile();
    std::vector<AudioChannelLayout> channels = alsa::getChannelMasksFromProfile(profile);
    std::vector<int> sampleRates = alsa::getSampleRatesFromProfile(profile);

    for (size_t i = 0; i < std::min(MAX_PROFILE_FORMATS, AUDIO_PORT_MAX_AUDIO_PROFILES) &&
                       profile->formats[i] != PCM_FORMAT_INVALID;
         ++i) {
        auto audioFormatDescription =
                alsa::c2aidl_pcm_format_AudioFormatDescription(profile->formats[i]);
        if (audioFormatDescription.type == AudioFormatType::DEFAULT) {
            LOG(WARNING) << __func__ << ": unknown pcm type=" << profile->formats[i];
            continue;
        }
        AudioProfile audioProfile = {.format = audioFormatDescription,
                                     .channelMasks = channels,
                                     .sampleRates = sampleRates};
        audioPort->profiles.push_back(std::move(audioProfile));
    }
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::audio::core
