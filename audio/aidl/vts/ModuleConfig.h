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

#pragma once

#include <functional>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include <aidl/android/hardware/audio/core/AudioRoute.h>
#include <aidl/android/hardware/audio/core/IModule.h>
#include <aidl/android/media/audio/common/AudioOffloadInfo.h>
#include <aidl/android/media/audio/common/AudioPort.h>

class ModuleConfig {
  public:
    using SrcSinkPair = std::pair<aidl::android::media::audio::common::AudioPortConfig,
                                  aidl::android::media::audio::common::AudioPortConfig>;
    using SrcSinkGroup =
            std::pair<aidl::android::hardware::audio::core::AudioRoute, std::vector<SrcSinkPair>>;

    static std::optional<aidl::android::media::audio::common::AudioOffloadInfo>
    generateOffloadInfoIfNeeded(
            const aidl::android::media::audio::common::AudioPortConfig& portConfig);

    std::vector<aidl::android::media::audio::common::AudioPort> getAudioPortsForDeviceTypes(
            const std::vector<aidl::android::media::audio::common::AudioDeviceType>& deviceTypes,
            const std::string& connection = "");
    static std::vector<aidl::android::media::audio::common::AudioPort> getAudioPortsForDeviceTypes(
            const std::vector<aidl::android::media::audio::common::AudioPort>& ports,
            const std::vector<aidl::android::media::audio::common::AudioDeviceType>& deviceTypes,
            const std::string& connection = "");
    static std::vector<aidl::android::media::audio::common::AudioPort> getBuiltInMicPorts(
            const std::vector<aidl::android::media::audio::common::AudioPort>& ports);

    explicit ModuleConfig(aidl::android::hardware::audio::core::IModule* module);
    const ndk::ScopedAStatus& getStatus() const { return mStatus; }
    std::string getError() const { return mStatus.getMessage(); }

    std::vector<aidl::android::media::audio::common::AudioPort> getAttachedDevicePorts() const;
    std::vector<aidl::android::media::audio::common::AudioPort> getConnectedExternalDevicePorts()
            const;
    std::set<int32_t> getConnectedSinkDevicePorts() const;
    std::set<int32_t> getConnectedSourceDevicePorts() const;
    std::vector<aidl::android::media::audio::common::AudioPort> getAttachedMicrophonePorts() const {
        return getBuiltInMicPorts(getAttachedDevicePorts());
    }
    std::vector<aidl::android::media::audio::common::AudioPort> getExternalDevicePorts() const;
    std::vector<aidl::android::media::audio::common::AudioPort> getInputMixPorts(
            bool connectedOnly /*Permanently attached and connected external devices*/) const;
    std::vector<aidl::android::media::audio::common::AudioPort> getOutputMixPorts(
            bool connectedOnly /*Permanently attached and connected external devices*/) const;
    std::vector<aidl::android::media::audio::common::AudioPort> getMixPorts(
            bool isInput,
            bool connectedOnly /*Permanently attached and connected external devices*/) const {
        return isInput ? getInputMixPorts(connectedOnly) : getOutputMixPorts(connectedOnly);
    }
    std::vector<aidl::android::media::audio::common::AudioPort> getNonBlockingMixPorts(
            bool connectedOnly /*Permanently attached and connected external devices*/,
            bool singlePort) const;
    std::vector<aidl::android::media::audio::common::AudioPort> getOffloadMixPorts(
            bool connectedOnly /*Permanently attached and connected external devices*/,
            bool singlePort) const;
    std::vector<aidl::android::media::audio::common::AudioPort> getPrimaryMixPorts(
            bool connectedOnly /*Permanently attached and connected external devices*/,
            bool singlePort) const;
    std::vector<aidl::android::media::audio::common::AudioPort> getMmapOutMixPorts(
            bool connectedOnly /*Permanently attached and connected external devices*/,
            bool singlePort) const;
    std::vector<aidl::android::media::audio::common::AudioPort> getMmapInMixPorts(
            bool connectedOnly /*Permanently attached and connected external devices*/,
            bool singlePort) const;

    std::vector<aidl::android::media::audio::common::AudioPort> getConnectedDevicesPortsForMixPort(
            bool isInput, const aidl::android::media::audio::common::AudioPort& mixPort) const {
        return isInput ? getConnectedSourceDevicesPortsForMixPort(mixPort)
                       : getConnectedSinkDevicesPortsForMixPort(mixPort);
    }
    std::vector<aidl::android::media::audio::common::AudioPort> getConnectedDevicesPortsForMixPort(
            bool isInput,
            const aidl::android::media::audio::common::AudioPortConfig& mixPortConfig) const;
    std::vector<aidl::android::media::audio::common::AudioPort>
    getConnectedSinkDevicesPortsForMixPort(
            const aidl::android::media::audio::common::AudioPort& mixPort) const;
    std::vector<aidl::android::media::audio::common::AudioPort>
    getConnectedSourceDevicesPortsForMixPort(
            const aidl::android::media::audio::common::AudioPort& mixPort) const;
    std::optional<aidl::android::media::audio::common::AudioPort>
    getSourceMixPortForConnectedDevice() const;

    std::optional<SrcSinkPair> getNonRoutableSrcSinkPair(bool isInput) const;
    std::optional<SrcSinkPair> getRoutableSrcSinkPair(bool isInput) const;
    std::vector<SrcSinkGroup> getRoutableSrcSinkGroups(bool isInput) const;

    std::vector<aidl::android::media::audio::common::AudioPortConfig>
    getPortConfigsForAttachedDevicePorts() const {
        return generateAudioDevicePortConfigs(getAttachedDevicePorts(), false);
    }
    std::vector<aidl::android::media::audio::common::AudioPortConfig> getPortConfigsForMixPorts()
            const {
        auto inputs =
                generateAudioMixPortConfigs(getInputMixPorts(false /*connectedOnly*/), true, false);
        auto outputs = generateAudioMixPortConfigs(getOutputMixPorts(false /*connectedOnly*/),
                                                   false, false);
        inputs.insert(inputs.end(), outputs.begin(), outputs.end());
        return inputs;
    }
    std::vector<aidl::android::media::audio::common::AudioPortConfig> getPortConfigsForMixPorts(
            bool isInput) const {
        return generateAudioMixPortConfigs(getMixPorts(isInput, false /*connectedOnly*/), isInput,
                                           false);
    }
    std::vector<aidl::android::media::audio::common::AudioPortConfig> getPortConfigsForMixPorts(
            bool isInput, const aidl::android::media::audio::common::AudioPort& port) const {
        return generateAudioMixPortConfigs({port}, isInput, false);
    }
    std::optional<aidl::android::media::audio::common::AudioPortConfig> getSingleConfigForMixPort(
            bool isInput) const {
        const auto config = generateAudioMixPortConfigs(
                getMixPorts(isInput, false /*connectedOnly*/), isInput, true);
        if (!config.empty()) {
            return *config.begin();
        }
        return {};
    }
    std::optional<aidl::android::media::audio::common::AudioPortConfig> getSingleConfigForMixPort(
            bool isInput, const aidl::android::media::audio::common::AudioPort& port) const {
        const auto config = generateAudioMixPortConfigs({port}, isInput, true);
        if (!config.empty()) {
            return *config.begin();
        }
        return {};
    }

    std::vector<aidl::android::media::audio::common::AudioPortConfig> getPortConfigsForDevicePort(
            const aidl::android::media::audio::common::AudioPort& port) const {
        return generateAudioDevicePortConfigs({port}, false);
    }
    aidl::android::media::audio::common::AudioPortConfig getSingleConfigForDevicePort(
            const aidl::android::media::audio::common::AudioPort& port) const {
        const auto config = generateAudioDevicePortConfigs({port}, true);
        return *config.begin();
    }

    const ndk::ScopedAStatus& onExternalDeviceConnected(
            aidl::android::hardware::audio::core::IModule* module,
            const aidl::android::media::audio::common::AudioPort& port);
    const ndk::ScopedAStatus& onExternalDeviceDisconnected(
            aidl::android::hardware::audio::core::IModule* module,
            const aidl::android::media::audio::common::AudioPort& port);

    bool isMmapSupported() const;

    std::string toString() const;

  private:
    std::vector<aidl::android::media::audio::common::AudioPort> findMixPorts(
            bool isInput, bool connectedOnly, bool singlePort,
            const std::function<bool(const aidl::android::media::audio::common::AudioPort&)>& pred)
            const;
    std::vector<aidl::android::media::audio::common::AudioPortConfig> generateAudioMixPortConfigs(
            const std::vector<aidl::android::media::audio::common::AudioPort>& ports, bool isInput,
            bool singleProfile) const;

    // Unlike MixPorts, the generator for DevicePorts always returns a non-empty
    // vector for a non-empty input port list. If there are no profiles in the
    // port, its initial configs are looked up, if there are none,
    // then an empty config is used, assuming further negotiation via setAudioPortConfig.
    std::vector<aidl::android::media::audio::common::AudioPortConfig>
    generateAudioDevicePortConfigs(
            const std::vector<aidl::android::media::audio::common::AudioPort>& ports,
            bool singleProfile) const;

    ndk::ScopedAStatus mStatus = ndk::ScopedAStatus::ok();
    std::vector<aidl::android::media::audio::common::AudioPort> mPorts;
    std::vector<aidl::android::media::audio::common::AudioPortConfig> mInitialConfigs;
    std::set<int32_t> mAttachedSinkDevicePorts;
    std::set<int32_t> mAttachedSourceDevicePorts;
    std::set<int32_t> mExternalDevicePorts;
    std::set<int32_t> mConnectedExternalSinkDevicePorts;
    std::set<int32_t> mConnectedExternalSourceDevicePorts;
    std::vector<aidl::android::hardware::audio::core::AudioRoute> mRoutes;
};
