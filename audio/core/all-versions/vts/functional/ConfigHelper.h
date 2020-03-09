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

// Code in this file uses 'getCachedPolicyConfig'
#ifndef AUDIO_PRIMARY_HIDL_HAL_TEST
#error Must be included from AudioPrimaryHidlTest.h
#endif

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
        if (policyConfig.getStatus() != OK || policyConfig.getPrimaryModule() == nullptr) {
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
    static const vector<AudioConfig> getRequiredSupportPlaybackAudioConfig() {
        return combineAudioConfig({AudioChannelMask::OUT_STEREO, AudioChannelMask::OUT_MONO},
                                  {8000, 11025, 16000, 22050, 32000, 44100},
                                  {AudioFormat::PCM_16_BIT});
    }

    static const vector<AudioConfig> getRecommendedSupportPlaybackAudioConfig() {
        return combineAudioConfig({AudioChannelMask::OUT_STEREO, AudioChannelMask::OUT_MONO},
                                  {24000, 48000}, {AudioFormat::PCM_16_BIT});
    }

    static const vector<AudioConfig> getRequiredSupportCaptureAudioConfig() {
        if (!primaryHasMic()) return {};
        return combineAudioConfig({AudioChannelMask::IN_MONO}, {8000, 11025, 16000, 44100},
                                  {AudioFormat::PCM_16_BIT});
    }
    static const vector<AudioConfig> getRecommendedSupportCaptureAudioConfig() {
        if (!primaryHasMic()) return {};
        return combineAudioConfig({AudioChannelMask::IN_STEREO}, {22050, 48000},
                                  {AudioFormat::PCM_16_BIT});
    }

    static vector<AudioConfig> combineAudioConfig(vector<audio_channel_mask_t> channelMasks,
                                                  vector<uint32_t> sampleRates,
                                                  audio_format_t format) {
        vector<AudioConfig> configs;
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

    static vector<AudioConfig> combineAudioConfig(vector<AudioChannelMask> channelMasks,
                                                  vector<uint32_t> sampleRates,
                                                  vector<AudioFormat> formats) {
        vector<AudioConfig> configs;
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
