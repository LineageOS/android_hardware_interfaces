/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <android/media/audio/common/AudioIoFlags.h>
#include <android/media/audio/common/AudioOutputFlags.h>

#include "ModuleConfig.h"

using namespace android;

using android::hardware::audio::core::IModule;
using android::media::audio::common::AudioChannelLayout;
using android::media::audio::common::AudioFormatDescription;
using android::media::audio::common::AudioFormatType;
using android::media::audio::common::AudioIoFlags;
using android::media::audio::common::AudioOutputFlags;
using android::media::audio::common::AudioPort;
using android::media::audio::common::AudioPortConfig;
using android::media::audio::common::AudioPortExt;
using android::media::audio::common::AudioProfile;
using android::media::audio::common::Int;

template <typename T>
auto findById(const std::vector<T>& v, int32_t id) {
    return std::find_if(v.begin(), v.end(), [&](const auto& p) { return p.id == id; });
}

ModuleConfig::ModuleConfig(IModule* module) {
    mStatus = module->getAudioPorts(&mPorts);
    if (!mStatus.isOk()) return;
    for (const auto& port : mPorts) {
        if (port.ext.getTag() != AudioPortExt::Tag::device) continue;
        const auto& devicePort = port.ext.get<AudioPortExt::Tag::device>();
        const bool isInput = port.flags.getTag() == AudioIoFlags::Tag::input;
        if (devicePort.device.type.connection.empty()) {
            // Permanently attached device.
            if (isInput) {
                mAttachedSourceDevicePorts.insert(port.id);
            } else {
                mAttachedSinkDevicePorts.insert(port.id);
            }
        }
    }
    if (!mStatus.isOk()) return;
    mStatus = module->getAudioRoutes(&mRoutes);
    if (!mStatus.isOk()) return;
    mStatus = module->getAudioPortConfigs(&mInitialConfigs);
}

std::vector<AudioPort> ModuleConfig::getInputMixPorts() const {
    std::vector<AudioPort> result;
    std::copy_if(mPorts.begin(), mPorts.end(), std::back_inserter(result), [](const auto& port) {
        return port.ext.getTag() == AudioPortExt::Tag::mix &&
               port.flags.getTag() == AudioIoFlags::Tag::input;
    });
    return result;
}

std::vector<AudioPort> ModuleConfig::getOutputMixPorts() const {
    std::vector<AudioPort> result;
    std::copy_if(mPorts.begin(), mPorts.end(), std::back_inserter(result), [](const auto& port) {
        return port.ext.getTag() == AudioPortExt::Tag::mix &&
               port.flags.getTag() == AudioIoFlags::Tag::output;
    });
    return result;
}

std::vector<AudioPort> ModuleConfig::getAttachedSinkDevicesPortsForMixPort(
        const AudioPort& mixPort) const {
    std::vector<AudioPort> result;
    for (const auto& route : mRoutes) {
        if (mAttachedSinkDevicePorts.count(route.sinkPortId) != 0 &&
            std::find(route.sourcePortIds.begin(), route.sourcePortIds.end(), mixPort.id) !=
                    route.sourcePortIds.end()) {
            const auto devicePortIt = findById<AudioPort>(mPorts, route.sinkPortId);
            if (devicePortIt != mPorts.end()) result.push_back(*devicePortIt);
        }
    }
    return result;
}

std::vector<AudioPort> ModuleConfig::getAttachedSourceDevicesPortsForMixPort(
        const AudioPort& mixPort) const {
    std::vector<AudioPort> result;
    for (const auto& route : mRoutes) {
        if (route.sinkPortId == mixPort.id) {
            for (const auto srcId : route.sourcePortIds) {
                if (mAttachedSourceDevicePorts.count(srcId) != 0) {
                    const auto devicePortIt = findById<AudioPort>(mPorts, srcId);
                    if (devicePortIt != mPorts.end()) result.push_back(*devicePortIt);
                }
            }
        }
    }
    return result;
}

std::optional<AudioPort> ModuleConfig::getSourceMixPortForAttachedDevice() const {
    for (const auto& route : mRoutes) {
        if (mAttachedSinkDevicePorts.count(route.sinkPortId) != 0) {
            const auto mixPortIt = findById<AudioPort>(mPorts, route.sourcePortIds[0]);
            if (mixPortIt != mPorts.end()) return *mixPortIt;
        }
    }
    return {};
}

std::optional<ModuleConfig::SrcSinkPair> ModuleConfig::getNonRoutableSrcSinkPair(
        bool isInput) const {
    const auto mixPorts = getMixPorts(isInput);
    std::set<std::pair<int32_t, int32_t>> allowedRoutes;
    for (const auto& route : mRoutes) {
        for (const auto srcPortId : route.sourcePortIds) {
            allowedRoutes.emplace(std::make_pair(srcPortId, route.sinkPortId));
        }
    }
    auto make_pair = [isInput](auto& device, auto& mix) {
        return isInput ? std::make_pair(device, mix) : std::make_pair(mix, device);
    };
    for (const auto portId : isInput ? mAttachedSourceDevicePorts : mAttachedSinkDevicePorts) {
        const auto devicePortIt = findById<AudioPort>(mPorts, portId);
        if (devicePortIt == mPorts.end()) continue;
        auto devicePortConfig = getSingleConfigForDevicePort(*devicePortIt);
        for (const auto& mixPort : mixPorts) {
            if (std::find(allowedRoutes.begin(), allowedRoutes.end(),
                          make_pair(portId, mixPort.id)) == allowedRoutes.end()) {
                auto mixPortConfig = getSingleConfigForMixPort(isInput, mixPort);
                if (mixPortConfig.has_value()) {
                    return make_pair(devicePortConfig, mixPortConfig.value());
                }
            }
        }
    }
    return {};
}

std::optional<ModuleConfig::SrcSinkPair> ModuleConfig::getRoutableSrcSinkPair(bool isInput) const {
    if (isInput) {
        for (const auto& route : mRoutes) {
            auto srcPortIdIt = std::find_if(
                    route.sourcePortIds.begin(), route.sourcePortIds.end(),
                    [&](const auto& portId) { return mAttachedSourceDevicePorts.count(portId); });
            if (srcPortIdIt == route.sourcePortIds.end()) continue;
            const auto devicePortIt = findById<AudioPort>(mPorts, *srcPortIdIt);
            const auto mixPortIt = findById<AudioPort>(mPorts, route.sinkPortId);
            if (devicePortIt == mPorts.end() || mixPortIt == mPorts.end()) continue;
            auto devicePortConfig = getSingleConfigForDevicePort(*devicePortIt);
            auto mixPortConfig = getSingleConfigForMixPort(isInput, *mixPortIt);
            if (!mixPortConfig.has_value()) continue;
            return std::make_pair(devicePortConfig, mixPortConfig.value());
        }
    } else {
        for (const auto& route : mRoutes) {
            if (mAttachedSinkDevicePorts.count(route.sinkPortId) == 0) continue;
            const auto mixPortIt = findById<AudioPort>(mPorts, route.sourcePortIds[0]);
            const auto devicePortIt = findById<AudioPort>(mPorts, route.sinkPortId);
            if (devicePortIt == mPorts.end() || mixPortIt == mPorts.end()) continue;
            auto mixPortConfig = getSingleConfigForMixPort(isInput, *mixPortIt);
            auto devicePortConfig = getSingleConfigForDevicePort(*devicePortIt);
            if (!mixPortConfig.has_value()) continue;
            return std::make_pair(mixPortConfig.value(), devicePortConfig);
        }
    }
    return {};
}

std::vector<ModuleConfig::SrcSinkGroup> ModuleConfig::getRoutableSrcSinkGroups(bool isInput) const {
    std::vector<SrcSinkGroup> result;
    if (isInput) {
        for (const auto& route : mRoutes) {
            std::vector<int32_t> srcPortIds;
            std::copy_if(route.sourcePortIds.begin(), route.sourcePortIds.end(),
                         std::back_inserter(srcPortIds), [&](const auto& portId) {
                             return mAttachedSourceDevicePorts.count(portId);
                         });
            if (srcPortIds.empty()) continue;
            const auto mixPortIt = findById<AudioPort>(mPorts, route.sinkPortId);
            if (mixPortIt == mPorts.end()) continue;
            auto mixPortConfig = getSingleConfigForMixPort(isInput, *mixPortIt);
            if (!mixPortConfig.has_value()) continue;
            std::vector<SrcSinkPair> pairs;
            for (const auto srcPortId : srcPortIds) {
                const auto devicePortIt = findById<AudioPort>(mPorts, srcPortId);
                if (devicePortIt == mPorts.end()) continue;
                // Using all configs for every source would be too much.
                auto devicePortConfig = getSingleConfigForDevicePort(*devicePortIt);
                pairs.emplace_back(devicePortConfig, mixPortConfig.value());
            }
            if (!pairs.empty()) {
                result.emplace_back(route, std::move(pairs));
            }
        }
    } else {
        for (const auto& route : mRoutes) {
            if (mAttachedSinkDevicePorts.count(route.sinkPortId) == 0) continue;
            const auto devicePortIt = findById<AudioPort>(mPorts, route.sinkPortId);
            if (devicePortIt == mPorts.end()) continue;
            auto devicePortConfig = getSingleConfigForDevicePort(*devicePortIt);
            std::vector<SrcSinkPair> pairs;
            for (const auto srcPortId : route.sourcePortIds) {
                const auto mixPortIt = findById<AudioPort>(mPorts, srcPortId);
                if (mixPortIt == mPorts.end()) continue;
                // Using all configs for every source would be too much.
                auto mixPortConfig = getSingleConfigForMixPort(isInput, *mixPortIt);
                if (mixPortConfig.has_value()) {
                    pairs.emplace_back(mixPortConfig.value(), devicePortConfig);
                }
            }
            if (!pairs.empty()) {
                result.emplace_back(route, std::move(pairs));
            }
        }
    }
    return result;
}

static std::vector<AudioPortConfig> combineAudioConfigs(const AudioPort& port,
                                                        const AudioProfile& profile) {
    std::vector<AudioPortConfig> configs;
    configs.reserve(profile.channelMasks.size() * profile.sampleRates.size());
    for (auto channelMask : profile.channelMasks) {
        for (auto sampleRate : profile.sampleRates) {
            AudioPortConfig config{};
            config.portId = port.id;
            Int sr;
            sr.value = sampleRate;
            config.sampleRate = sr;
            config.channelMask = channelMask;
            config.format = profile.format;
            config.ext = port.ext;
            configs.push_back(config);
        }
    }
    return configs;
}

std::vector<AudioPortConfig> ModuleConfig::generateInputAudioMixPortConfigs(
        const std::vector<AudioPort>& ports, bool singleProfile) const {
    std::vector<AudioPortConfig> result;
    for (const auto& mixPort : ports) {
        if (getAttachedSourceDevicesPortsForMixPort(mixPort).empty()) {
            continue;  // no attached devices
        }
        for (const auto& profile : mixPort.profiles) {
            if (profile.format.type == AudioFormatType::DEFAULT || profile.sampleRates.empty() ||
                profile.channelMasks.empty()) {
                continue;  // dynamic profile
            }
            auto configs = combineAudioConfigs(mixPort, profile);
            for (auto& config : configs) {
                config.flags = mixPort.flags;
                result.push_back(config);
                if (singleProfile) return result;
            }
        }
    }
    return result;
}

static std::tuple<AudioIoFlags, bool> generateOutFlags(const AudioPort& mixPort) {
    static const AudioIoFlags offloadFlags = AudioIoFlags::make<AudioIoFlags::Tag::output>(
            (1 << static_cast<int>(AudioOutputFlags::COMPRESS_OFFLOAD)) |
            (1 << static_cast<int>(AudioOutputFlags::DIRECT)));
    const bool isOffload = (mixPort.flags.get<AudioIoFlags::Tag::output>() &
                            (1 << static_cast<int>(AudioOutputFlags::COMPRESS_OFFLOAD))) != 0;
    return {isOffload ? offloadFlags : mixPort.flags, isOffload};
}

std::vector<AudioPortConfig> ModuleConfig::generateOutputAudioMixPortConfigs(
        const std::vector<AudioPort>& ports, bool singleProfile) const {
    std::vector<AudioPortConfig> result;
    for (const auto& mixPort : ports) {
        if (getAttachedSinkDevicesPortsForMixPort(mixPort).empty()) {
            continue;  // no attached devices
        }
        auto [flags, isOffload] = generateOutFlags(mixPort);
        (void)isOffload;
        for (const auto& profile : mixPort.profiles) {
            if (profile.format.type == AudioFormatType::DEFAULT) continue;
            auto configs = combineAudioConfigs(mixPort, profile);
            for (auto& config : configs) {
                // Some combinations of flags declared in the config file require special
                // treatment.
                // if (isOffload) {
                //     config.offloadInfo.info(generateOffloadInfo(config.base));
                // }
                config.flags = flags;
                result.push_back(config);
                if (singleProfile) return result;
            }
        }
    }
    return result;
}

std::vector<AudioPortConfig> ModuleConfig::generateAudioDevicePortConfigs(
        const std::vector<AudioPort>& ports, bool singleProfile) const {
    std::vector<AudioPortConfig> result;
    for (const auto& devicePort : ports) {
        const size_t resultSizeBefore = result.size();
        for (const auto& profile : devicePort.profiles) {
            auto configs = combineAudioConfigs(devicePort, profile);
            result.insert(result.end(), configs.begin(), configs.end());
            if (singleProfile && !result.empty()) return result;
        }
        if (resultSizeBefore == result.size()) {
            AudioPortConfig empty;
            empty.portId = devicePort.id;
            empty.ext = devicePort.ext;
            result.push_back(empty);
        }
        if (singleProfile) return result;
    }
    return result;
}
