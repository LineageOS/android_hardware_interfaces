/*
 * Copyright (C) 2020 The Android Open Source Project
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

// pull in all the <= 6.0 tests
#include "6.0/AudioPrimaryHidlHalTest.cpp"

static std::vector<AudioConfig> combineAudioConfig(std::vector<xsd::AudioChannelMask> channelMasks,
                                                   std::vector<int64_t> sampleRates,
                                                   const std::string& format) {
    std::vector<AudioConfig> configs;
    configs.reserve(channelMasks.size() * sampleRates.size());
    for (auto channelMask : channelMasks) {
        for (auto sampleRate : sampleRates) {
            AudioConfig config{};
            // leave offloadInfo to 0
            config.base.channelMask = toString(channelMask);
            config.base.sampleRateHz = sampleRate;
            config.base.format = format;
            configs.push_back(config);
        }
    }
    return configs;
}

const std::vector<DeviceConfigParameter>& getOutputDeviceConfigParameters() {
    static std::vector<DeviceConfigParameter> parameters = [] {
        std::vector<DeviceConfigParameter> result;
        const std::vector<AudioInOutFlag> offloadFlags = {
                toString(xsd::AudioInOutFlag::AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD),
                toString(xsd::AudioInOutFlag::AUDIO_OUTPUT_FLAG_DIRECT)};
        for (const auto& device : getDeviceParameters()) {
            auto module =
                    getCachedPolicyConfig().getModuleFromName(std::get<PARAM_DEVICE_NAME>(device));
            if (!module || !module->getFirstMixPorts()) break;
            for (const auto& mixPort : module->getFirstMixPorts()->getMixPort()) {
                if (mixPort.getRole() != xsd::Role::source) continue;  // not an output profile
                std::vector<AudioInOutFlag> flags;
                bool isOffload = false;
                if (mixPort.hasFlags()) {
                    auto xsdFlags = mixPort.getFlags();
                    isOffload =
                            std::find(xsdFlags.begin(), xsdFlags.end(),
                                      xsd::AudioInOutFlag::AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD) !=
                            xsdFlags.end();
                    if (!isOffload) {
                        for (auto flag : xsdFlags) {
                            if (flag != xsd::AudioInOutFlag::AUDIO_OUTPUT_FLAG_PRIMARY) {
                                flags.push_back(toString(flag));
                            }
                        }
                    } else {
                        flags = offloadFlags;
                    }
                }
                for (const auto& profile : mixPort.getProfile()) {
                    auto configs =
                            combineAudioConfig(profile.getChannelMasks(),
                                               profile.getSamplingRates(), profile.getFormat());
                    for (auto& config : configs) {
                        // Some combinations of flags declared in the config file require special
                        // treatment.
                        if (isOffload) {
                            config.offloadInfo.base = config.base;
                            config.offloadInfo.streamType =
                                    toString(xsd::AudioStreamType::AUDIO_STREAM_MUSIC);
                            config.offloadInfo.usage = toString(xsd::AudioUsage::AUDIO_USAGE_MEDIA);
                            config.offloadInfo.bitRatePerSecond = 320;
                            config.offloadInfo.durationMicroseconds = -1;
                            config.offloadInfo.bitWidth = 16;
                            config.offloadInfo.bufferSize = 256;  // arbitrary value
                        }
                        result.emplace_back(device, config, flags);
                    }
                }
            }
        }
        return result;
    }();
    return parameters;
}

const std::vector<DeviceConfigParameter>& getInputDeviceConfigParameters() {
    static std::vector<DeviceConfigParameter> parameters = [] {
        std::vector<DeviceConfigParameter> result;
        for (const auto& device : getDeviceParameters()) {
            auto module =
                    getCachedPolicyConfig().getModuleFromName(std::get<PARAM_DEVICE_NAME>(device));
            if (!module || !module->getFirstMixPorts()) break;
            for (const auto& mixPort : module->getFirstMixPorts()->getMixPort()) {
                if (mixPort.getRole() != xsd::Role::sink) continue;  // not an input profile
                std::vector<AudioInOutFlag> flags;
                if (mixPort.hasFlags()) {
                    std::transform(mixPort.getFlags().begin(), mixPort.getFlags().end(),
                                   std::back_inserter(flags),
                                   [](auto flag) { return toString(flag); });
                }
                for (const auto& profile : mixPort.getProfile()) {
                    auto configs =
                            combineAudioConfig(profile.getChannelMasks(),
                                               profile.getSamplingRates(), profile.getFormat());
                    for (const auto& config : configs) {
                        result.emplace_back(device, config, flags);
                    }
                }
            }
        }
        return result;
    }();
    return parameters;
}
