/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <common/all-versions/VersionUtils.h>

#include "PolicyConfig.h"

// clang-format off
#include PATH(android/hardware/audio/FILE_VERSION/types.h)
#include PATH(android/hardware/audio/common/FILE_VERSION/types.h)
// clang-format on

using ::android::hardware::audio::common::utils::EnumBitfield;
using ::android::hardware::audio::common::utils::mkEnumBitfield;

// Forward declaration for functions that are substituted
// in generator unit tests.
const PolicyConfig& getCachedPolicyConfig();

//////////////////////////////////////////////////////////////////////////////
//////////////// Required and recommended audio format support ///////////////
// From:
// https://source.android.com/compatibility/android-cdd.html#5_4_audio_recording
// From:
// https://source.android.com/compatibility/android-cdd.html#5_5_audio_playback
/////////// TODO: move to the beginning of the file for easier update ////////
//////////////////////////////////////////////////////////////////////////////

struct ConfigHelper {
    // for retro compatibility only test the primary device IN_BUILTIN_MIC
    // FIXME: in the next audio HAL version, test all available devices
    static bool primaryHasMic() {
        auto& policyConfig = getCachedPolicyConfig();
        if (policyConfig.getStatus() != android::OK || policyConfig.getPrimaryModule() == nullptr) {
            return true;  // Could not get the information, run all tests
        }
        auto getMic = [](auto& devs) {
            return devs.getDevice(AUDIO_DEVICE_IN_BUILTIN_MIC, {}, AUDIO_FORMAT_DEFAULT);
        };
        auto primaryMic = getMic(policyConfig.getPrimaryModule()->getDeclaredDevices());
        auto availableMic = getMic(policyConfig.getInputDevices());

        return primaryMic != nullptr && primaryMic->equals(availableMic);
    }

    // Cache result ?
    static const std::vector<AudioConfig> getRequiredSupportPlaybackAudioConfig() {
        return combineAudioConfig({AudioChannelMask::OUT_STEREO, AudioChannelMask::OUT_MONO},
                                  {8000, 11025, 16000, 22050, 32000, 44100},
                                  {AudioFormat::PCM_16_BIT});
    }

    static const std::vector<AudioConfig> getRecommendedSupportPlaybackAudioConfig() {
        return combineAudioConfig({AudioChannelMask::OUT_STEREO, AudioChannelMask::OUT_MONO},
                                  {24000, 48000}, {AudioFormat::PCM_16_BIT});
    }

    static const std::vector<AudioConfig> getRequiredSupportCaptureAudioConfig() {
        if (!primaryHasMic()) return {};
        return combineAudioConfig({AudioChannelMask::IN_MONO}, {8000, 11025, 16000, 44100},
                                  {AudioFormat::PCM_16_BIT});
    }
    static const std::vector<AudioConfig> getRecommendedSupportCaptureAudioConfig() {
        if (!primaryHasMic()) return {};
        return combineAudioConfig({AudioChannelMask::IN_STEREO}, {22050, 48000},
                                  {AudioFormat::PCM_16_BIT});
    }

    static std::vector<AudioConfig> combineAudioConfig(
            std::vector<audio_channel_mask_t> channelMasks, std::vector<uint32_t> sampleRates,
            audio_format_t format) {
        std::vector<AudioConfig> configs;
        configs.reserve(channelMasks.size() * sampleRates.size());
        for (auto channelMask : channelMasks) {
            for (auto sampleRate : sampleRates) {
                AudioConfig config{};
                // leave offloadInfo to 0
                config.channelMask = EnumBitfield<AudioChannelMask>(channelMask);
                config.sampleRateHz = sampleRate;
                config.format = AudioFormat(format);
                configs.push_back(config);
            }
        }
        return configs;
    }

    static std::vector<AudioConfig> combineAudioConfig(std::vector<AudioChannelMask> channelMasks,
                                                       std::vector<uint32_t> sampleRates,
                                                       std::vector<AudioFormat> formats) {
        std::vector<AudioConfig> configs;
        configs.reserve(channelMasks.size() * sampleRates.size() * formats.size());
        for (auto channelMask : channelMasks) {
            for (auto sampleRate : sampleRates) {
                for (auto format : formats) {
                    AudioConfig config{};
                    // leave offloadInfo to 0
                    config.channelMask = mkEnumBitfield(channelMask);
                    config.sampleRateHz = sampleRate;
                    config.format = format;
                    // FIXME: leave frameCount to 0 ?
                    configs.push_back(config);
                }
            }
        }
        return configs;
    }
};
