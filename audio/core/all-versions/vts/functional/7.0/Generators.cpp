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

#include "7.0/Generators.h"
#include "7.0/PolicyConfig.h"

// clang-format off
#include PATH(android/hardware/audio/CORE_TYPES_FILE_VERSION/types.h)
#include PATH(android/hardware/audio/common/COMMON_TYPES_FILE_VERSION/types.h)
// clang-format on

#include PATH(APM_XSD_ENUMS_H_FILENAME)
#include PATH(APM_XSD_H_FILENAME)

// Forward declaration for functions that are substituted
// in generator unit tests.
const PolicyConfig& getCachedPolicyConfig();
const std::vector<DeviceParameter>& getDeviceParameters();

using namespace ::android::hardware::audio::common::COMMON_TYPES_CPP_VERSION;
using namespace ::android::hardware::audio::CORE_TYPES_CPP_VERSION;
namespace xsd {
using namespace ::android::audio::policy::configuration::CPP_VERSION;
}

static std::vector<AudioConfig> combineAudioConfig(std::vector<xsd::AudioChannelMask> channelMasks,
                                                   std::vector<int64_t> sampleRates,
                                                   const std::string& format) {
    std::vector<AudioConfig> configs;
    configs.reserve(channelMasks.size() * sampleRates.size());
    for (auto channelMask : channelMasks) {
        for (auto sampleRate : sampleRates) {
            AudioConfig config{};
            config.base.channelMask = toString(channelMask);
            config.base.sampleRateHz = sampleRate;
            config.base.format = format;
            configs.push_back(config);
        }
    }
    return configs;
}

static std::tuple<std::vector<AudioInOutFlag>, bool> generateOutFlags(
        const xsd::MixPorts::MixPort& mixPort) {
    std::vector<AudioInOutFlag> flags;
    bool isOffload = false;
    if (mixPort.hasFlags()) {
        auto xsdFlags = mixPort.getFlags();
        isOffload = std::find(xsdFlags.begin(), xsdFlags.end(),
                              xsd::AudioInOutFlag::AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD) !=
                    xsdFlags.end();
        for (auto flag : xsdFlags) {
            if (flag != xsd::AudioInOutFlag::AUDIO_OUTPUT_FLAG_PRIMARY) {
                flags.push_back(toString(flag));
            }
        }
    }
    return {flags, isOffload};
}

static AudioOffloadInfo generateOffloadInfo(const AudioConfigBase& base) {
    return AudioOffloadInfo{
            .base = base,
            .streamType = toString(xsd::AudioStreamType::AUDIO_STREAM_MUSIC),
            .usage = toString(xsd::AudioUsage::AUDIO_USAGE_MEDIA),
            .bitRatePerSecond = 192,  // as in sine882hz3s.mp3
            .durationMicroseconds = -1,
            .bitWidth = 16,
            .bufferSize = 72000  // 3 seconds at 192 kbps, as in sine882hz3s.mp3
    };
}

std::vector<DeviceConfigParameter> generateOutputDeviceConfigParameters(bool oneProfilePerDevice) {
    std::vector<DeviceConfigParameter> result;
    for (const auto& device : getDeviceParameters()) {
        const std::string moduleName = std::get<PARAM_DEVICE_NAME>(device);
        auto module = getCachedPolicyConfig().getModuleFromName(moduleName);
        if (!module || !module->getFirstMixPorts()) break;
        for (const auto& mixPort : module->getFirstMixPorts()->getMixPort()) {
            if (mixPort.getRole() != xsd::Role::source) continue;  // not an output profile
            const auto attachedDeviceAddress =
                    getCachedPolicyConfig().getDeviceAddressOfSinkDeviceAttachedToMixPort(
                            moduleName, mixPort.getName());
            if (!attachedDeviceAddress.has_value()) continue;
            auto [flags, isOffload] = generateOutFlags(mixPort);
            for (const auto& profile : mixPort.getProfile()) {
                if (!profile.hasFormat() || !profile.hasSamplingRates() ||
                    !profile.hasChannelMasks())
                    continue;
                auto configs = combineAudioConfig(profile.getChannelMasks(),
                                                  profile.getSamplingRates(), profile.getFormat());
                for (auto& config : configs) {
                    // Some combinations of flags declared in the config file require special
                    // treatment.
                    if (isOffload) {
                        config.offloadInfo.info(generateOffloadInfo(config.base));
                    }
                    result.emplace_back(device, mixPort.getName(), attachedDeviceAddress.value(),
                                        config, flags);
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

const std::vector<DeviceConfigParameter>& getOutputDeviceInvalidConfigParameters(
        bool generateInvalidFlags) {
    static std::vector<DeviceConfigParameter> parameters = [&] {
        std::vector<DeviceConfigParameter> result;
        for (const auto& device : getDeviceParameters()) {
            auto module =
                    getCachedPolicyConfig().getModuleFromName(std::get<PARAM_DEVICE_NAME>(device));
            if (!module || !module->getFirstMixPorts()) break;
            bool hasRegularConfig = false, hasOffloadConfig = false;
            for (const auto& mixPort : module->getFirstMixPorts()->getMixPort()) {
                if (mixPort.getRole() != xsd::Role::source) continue;  // not an output profile
                auto [validFlags, isOffload] = generateOutFlags(mixPort);
                if ((!isOffload && hasRegularConfig) || (isOffload && hasOffloadConfig)) continue;
                for (const auto& profile : mixPort.getProfile()) {
                    if (!profile.hasFormat() || !profile.hasSamplingRates() ||
                        !profile.hasChannelMasks())
                        continue;
                    AudioConfigBase validBase = {
                            profile.getFormat(),
                            static_cast<uint32_t>(profile.getSamplingRates()[0]),
                            toString(profile.getChannelMasks()[0])};
                    DeviceAddress defaultDevice = {
                            toString(xsd::AudioDevice::AUDIO_DEVICE_OUT_DEFAULT), {}};
                    {
                        AudioConfig config{.base = validBase};
                        config.base.channelMask = "random_string";
                        if (isOffload) {
                            config.offloadInfo.info(generateOffloadInfo(validBase));
                        }
                        result.emplace_back(device, mixPort.getName(), defaultDevice, config,
                                            validFlags);
                    }
                    {
                        AudioConfig config{.base = validBase};
                        config.base.format = "random_string";
                        if (isOffload) {
                            config.offloadInfo.info(generateOffloadInfo(validBase));
                        }
                        result.emplace_back(device, mixPort.getName(), defaultDevice, config,
                                            validFlags);
                    }
                    if (generateInvalidFlags) {
                        AudioConfig config{.base = validBase};
                        if (isOffload) {
                            config.offloadInfo.info(generateOffloadInfo(validBase));
                        }
                        std::vector<AudioInOutFlag> flags = {"random_string", ""};
                        result.emplace_back(device, mixPort.getName(), defaultDevice, config,
                                            flags);
                    }
                    if (isOffload) {
                        {
                            AudioConfig config{.base = validBase};
                            config.offloadInfo.info(generateOffloadInfo(validBase));
                            config.offloadInfo.info().base.channelMask = "random_string";
                            result.emplace_back(device, mixPort.getName(), defaultDevice, config,
                                                validFlags);
                        }
                        {
                            AudioConfig config{.base = validBase};
                            config.offloadInfo.info(generateOffloadInfo(validBase));
                            config.offloadInfo.info().base.format = "random_string";
                            result.emplace_back(device, mixPort.getName(), defaultDevice, config,
                                                validFlags);
                        }
                        {
                            AudioConfig config{.base = validBase};
                            config.offloadInfo.info(generateOffloadInfo(validBase));
                            config.offloadInfo.info().streamType = "random_string";
                            result.emplace_back(device, mixPort.getName(), defaultDevice, config,
                                                validFlags);
                        }
                        {
                            AudioConfig config{.base = validBase};
                            config.offloadInfo.info(generateOffloadInfo(validBase));
                            config.offloadInfo.info().usage = "random_string";
                            result.emplace_back(device, mixPort.getName(), defaultDevice, config,
                                                validFlags);
                        }
                        hasOffloadConfig = true;
                    } else {
                        hasRegularConfig = true;
                    }
                    break;
                }
                if (hasOffloadConfig && hasRegularConfig) break;
            }
        }
        return result;
    }();
    return parameters;
}

std::vector<DeviceConfigParameter> generateInputDeviceConfigParameters(bool oneProfilePerDevice) {
    std::vector<DeviceConfigParameter> result;
    for (const auto& device : getDeviceParameters()) {
        const std::string moduleName = std::get<PARAM_DEVICE_NAME>(device);
        auto module = getCachedPolicyConfig().getModuleFromName(moduleName);
        if (!module || !module->getFirstMixPorts()) break;
        for (const auto& mixPort : module->getFirstMixPorts()->getMixPort()) {
            if (mixPort.getRole() != xsd::Role::sink) continue;  // not an input profile
            const auto attachedDeviceAddress =
                    getCachedPolicyConfig().getDeviceAddressOfSourceDeviceAttachedToMixPort(
                            moduleName, mixPort.getName());
            if (!attachedDeviceAddress.has_value()) continue;
            std::vector<AudioInOutFlag> flags;
            if (mixPort.hasFlags()) {
                std::transform(mixPort.getFlags().begin(), mixPort.getFlags().end(),
                               std::back_inserter(flags), [](auto flag) { return toString(flag); });
            }
            for (const auto& profile : mixPort.getProfile()) {
                if (!profile.hasFormat() || !profile.hasSamplingRates() ||
                    !profile.hasChannelMasks())
                    continue;
                auto configs = combineAudioConfig(profile.getChannelMasks(),
                                                  profile.getSamplingRates(), profile.getFormat());
                for (const auto& config : configs) {
                    result.emplace_back(device, mixPort.getName(), attachedDeviceAddress.value(),
                                        config, flags);
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

const std::vector<DeviceConfigParameter>& getInputDeviceInvalidConfigParameters(
        bool generateInvalidFlags) {
    static std::vector<DeviceConfigParameter> parameters = [&] {
        std::vector<DeviceConfigParameter> result;
        for (const auto& device : getDeviceParameters()) {
            auto module =
                    getCachedPolicyConfig().getModuleFromName(std::get<PARAM_DEVICE_NAME>(device));
            if (!module || !module->getFirstMixPorts()) break;
            bool hasConfig = false;
            for (const auto& mixPort : module->getFirstMixPorts()->getMixPort()) {
                if (mixPort.getRole() != xsd::Role::sink) continue;  // not an input profile
                std::vector<AudioInOutFlag> validFlags;
                if (mixPort.hasFlags()) {
                    std::transform(mixPort.getFlags().begin(), mixPort.getFlags().end(),
                                   std::back_inserter(validFlags),
                                   [](auto flag) { return toString(flag); });
                }
                for (const auto& profile : mixPort.getProfile()) {
                    if (!profile.hasFormat() || !profile.hasSamplingRates() ||
                        !profile.hasChannelMasks())
                        continue;
                    AudioConfigBase validBase = {
                            profile.getFormat(),
                            static_cast<uint32_t>(profile.getSamplingRates()[0]),
                            toString(profile.getChannelMasks()[0])};
                    DeviceAddress defaultDevice = {
                            toString(xsd::AudioDevice::AUDIO_DEVICE_IN_DEFAULT), {}};
                    {
                        AudioConfig config{.base = validBase};
                        config.base.channelMask = "random_string";
                        result.emplace_back(device, mixPort.getName(), defaultDevice, config,
                                            validFlags);
                    }
                    {
                        AudioConfig config{.base = validBase};
                        config.base.format = "random_string";
                        result.emplace_back(device, mixPort.getName(), defaultDevice, config,
                                            validFlags);
                    }
                    if (generateInvalidFlags) {
                        AudioConfig config{.base = validBase};
                        std::vector<AudioInOutFlag> flags = {"random_string", ""};
                        result.emplace_back(device, mixPort.getName(), defaultDevice, config,
                                            flags);
                    }
                    hasConfig = true;
                    break;
                }
                if (hasConfig) break;
            }
        }
        return result;
    }();
    return parameters;
}
