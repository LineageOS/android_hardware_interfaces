/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include <android-base/macros.h>

#include "6.0/Generators.h"
#include "ConfigHelper.h"
#include "PolicyConfig.h"

// clang-format off
#include PATH(android/hardware/audio/FILE_VERSION/types.h)
#include PATH(android/hardware/audio/common/FILE_VERSION/types.h)
// clang-format on

// Forward declaration for functions that are substituted
// in generator unit tests.
const PolicyConfig& getCachedPolicyConfig();
const std::vector<DeviceParameter>& getDeviceParameters();

using namespace ::android::hardware::audio::common::CPP_VERSION;
using namespace ::android::hardware::audio::CPP_VERSION;

std::vector<DeviceConfigParameter> generateOutputDeviceConfigParameters(bool oneProfilePerDevice) {
    std::vector<DeviceConfigParameter> result;
    for (const auto& device : getDeviceParameters()) {
        const std::string moduleName = std::get<PARAM_DEVICE_NAME>(device);
        auto module = getCachedPolicyConfig().getModuleFromName(moduleName);
        for (const auto& ioProfile : module->getOutputProfiles()) {
            if (getCachedPolicyConfig()
                        .getAttachedSinkDeviceForMixPort(moduleName, ioProfile->getName())
                        .empty()) {
                continue;  // no attached device
            }
            for (const auto& profile : ioProfile->getAudioProfiles()) {
                const auto& channels = profile->getChannels();
                const auto& sampleRates = profile->getSampleRates();
                auto configs = ConfigHelper::combineAudioConfig(
                        std::vector<audio_channel_mask_t>(channels.begin(), channels.end()),
                        std::vector<uint32_t>(sampleRates.begin(), sampleRates.end()),
                        profile->getFormat());
                auto flags = ioProfile->getFlags();
                for (auto& config : configs) {
                    // Some combinations of flags declared in the config file require special
                    // treatment.
                    if (flags & AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD) {
                        config.offloadInfo.sampleRateHz = config.sampleRateHz;
                        config.offloadInfo.channelMask = config.channelMask;
                        config.offloadInfo.format = config.format;
                        config.offloadInfo.streamType = AudioStreamType::MUSIC;
                        config.offloadInfo.bitRatePerSecond = 320;
                        config.offloadInfo.durationMicroseconds = -1;
                        config.offloadInfo.bitWidth = 16;
                        config.offloadInfo.bufferSize = 256;  // arbitrary value
                        config.offloadInfo.usage = AudioUsage::MEDIA;
                        result.emplace_back(device, config,
                                            AudioOutputFlag(AudioOutputFlag::COMPRESS_OFFLOAD |
                                                            AudioOutputFlag::DIRECT));
                    } else {
                        if (flags & AUDIO_OUTPUT_FLAG_PRIMARY) {  // ignore the flag
                            flags &= ~AUDIO_OUTPUT_FLAG_PRIMARY;
                        }
                        result.emplace_back(device, config, AudioOutputFlag(flags));
                    }
                    if (oneProfilePerDevice) break;
                }
                if (oneProfilePerDevice) break;
            }
            if (oneProfilePerDevice) break;
        }
    }
    return result;
}

const std::vector<DeviceConfigParameter>& getOutputDeviceConfigParameters() {
    static std::vector<DeviceConfigParameter> parameters =
            generateOutputDeviceConfigParameters(false);
    return parameters;
}

const std::vector<DeviceConfigParameter>& getOutputDeviceSingleConfigParameters() {
    static std::vector<DeviceConfigParameter> parameters =
            generateOutputDeviceConfigParameters(true);
    return parameters;
}

std::vector<DeviceConfigParameter> generateInputDeviceConfigParameters(bool oneProfilePerDevice) {
    std::vector<DeviceConfigParameter> result;
    for (const auto& device : getDeviceParameters()) {
        const std::string moduleName = std::get<PARAM_DEVICE_NAME>(device);
        auto module = getCachedPolicyConfig().getModuleFromName(moduleName);
        for (const auto& ioProfile : module->getInputProfiles()) {
            if (getCachedPolicyConfig()
                        .getAttachedSourceDeviceForMixPort(moduleName, ioProfile->getName())
                        .empty()) {
                continue;  // no attached device
            }
            for (const auto& profile : ioProfile->getAudioProfiles()) {
                const auto& channels = profile->getChannels();
                const auto& sampleRates = profile->getSampleRates();
                auto configs = ConfigHelper::combineAudioConfig(
                        std::vector<audio_channel_mask_t>(channels.begin(), channels.end()),
                        std::vector<uint32_t>(sampleRates.begin(), sampleRates.end()),
                        profile->getFormat());
                for (const auto& config : configs) {
                    result.emplace_back(device, config, AudioInputFlag(ioProfile->getFlags()));
                    if (oneProfilePerDevice) break;
                }
                if (oneProfilePerDevice) break;
            }
            if (oneProfilePerDevice) break;
        }
    }
    return result;
}

const std::vector<DeviceConfigParameter>& getInputDeviceConfigParameters() {
    static std::vector<DeviceConfigParameter> parameters =
            generateInputDeviceConfigParameters(false);
    return parameters;
}

const std::vector<DeviceConfigParameter>& getInputDeviceSingleConfigParameters() {
    static std::vector<DeviceConfigParameter> parameters =
            generateInputDeviceConfigParameters(true);
    return parameters;
}
