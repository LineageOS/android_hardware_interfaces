/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include "../include/AudioControlTestUtils.h"

#include <set>

using android::hardware::automotive::audiocontrol::AudioZone;
using android::hardware::automotive::audiocontrol::AudioZoneConfig;
using android::hardware::automotive::audiocontrol::AudioZoneContext;
using android::hardware::automotive::audiocontrol::AudioZoneContextInfo;
using android::hardware::automotive::audiocontrol::DeviceToContextEntry;
using android::hardware::automotive::audiocontrol::VolumeGroupConfig;

namespace audiomediacommon = android::media::audio::common;

namespace android {
namespace hardware {
namespace audiocontrol {
namespace testutils {

std::string toAlphaNumeric(const std::string& info) {
    std::string name = info;
    for (size_t i = 0; i < name.size(); i++) {
        // gtest test names must only contain alphanumeric characters
        if (!std::isalnum(name[i])) name[i] = '_';
    }

    return name;
}

bool getAudioPortDeviceDescriptor(const audiomediacommon::AudioPort& audioPort,
                                  audiomediacommon::AudioDeviceDescription& description) {
    if (audioPort.ext.getTag() != audiomediacommon::AudioPortExt::Tag::device) {
        return false;
    }
    const auto& audioDevice =
            audioPort.ext.get<audiomediacommon::AudioPortExt::Tag::device>().device;
    description = audioDevice.type;
    return true;
}

bool getAddressForAudioPort(const android::media::audio::common::AudioPort& audioPort,
                            std::string& address) {
    if (audioPort.ext.getTag() != audiomediacommon::AudioPortExt::Tag::device) {
        return false;
    }
    const auto& audioDevice =
            audioPort.ext.get<audiomediacommon::AudioPortExt::Tag::device>().device;

    switch (audioDevice.address.getTag()) {
        case audiomediacommon::AudioDeviceAddress::Tag::id:
            address = audioDevice.address.get<audiomediacommon::AudioDeviceAddress::Tag::id>();
            return true;
        case audiomediacommon::AudioDeviceAddress::Tag::alsa:
            address = android::internal::ToString(
                    audioDevice.address.get<audiomediacommon::AudioDeviceAddress::Tag::alsa>());
            return true;
        case audiomediacommon::AudioDeviceAddress::Tag::mac:
            address = android::internal::ToString(
                    audioDevice.address.get<audiomediacommon::AudioDeviceAddress::Tag::mac>());
            return true;
        case audiomediacommon::AudioDeviceAddress::Tag::ipv4:
            address = android::internal::ToString(
                    audioDevice.address.get<audiomediacommon::AudioDeviceAddress::Tag::ipv4>());
            return true;
        case audiomediacommon::AudioDeviceAddress::Tag::ipv6:
            address = android::internal::ToString(
                    audioDevice.address.get<audiomediacommon::AudioDeviceAddress::Tag::ipv6>());
            return true;
        default:
            address = audioDevice.address.toString();
            return true;
    }
}

bool getAddressForAudioDevice(const DeviceToContextEntry& device, std::string& address) {
    if (device.device.flags.getTag() == audiomediacommon::AudioIoFlags::input ||
        device.device.ext.getTag() != audiomediacommon::AudioPortExt::Tag::device) {
        return false;
    }
    return getAddressForAudioPort(device.device, address);
}

std::vector<std::string> getDeviceAddressesForVolumeGroup(const VolumeGroupConfig& config) {
    std::vector<std::string> addresses;
    for (const auto& route : config.carAudioRoutes) {
        std::string address;
        if (!getAddressForAudioDevice(route, address)) {
            continue;
        }
        addresses.push_back(address);
    }
    return addresses;
}

std::vector<std::string> getDeviceAddressesForZoneConfig(const AudioZoneConfig& config) {
    std::vector<std::string> addresses;
    for (const auto& volumeGroup : config.volumeGroups) {
        const auto groupAddresses = getDeviceAddressesForVolumeGroup(volumeGroup);
        addresses.insert(addresses.begin(), groupAddresses.begin(), groupAddresses.end());
    }
    return addresses;
}

std::vector<std::string> getDeviceAddressesForZone(const AudioZone& config) {
    std::vector<std::string> addresses;
    for (const auto& zoneConfig : config.audioZoneConfigs) {
        const auto groupAddresses = getDeviceAddressesForZoneConfig(zoneConfig);
        addresses.insert(addresses.begin(), groupAddresses.begin(), groupAddresses.end());
    }
    return addresses;
}

static void addContextUsages(const AudioZoneContextInfo& info,
                             std::set<audiomediacommon::AudioUsage>& contextUsages) {
    for (const auto& audioAttribute : info.audioAttributes) {
        contextUsages.insert(audioAttribute.usage);
    }
}

bool contextInfosContainAllAudioAttributeUsages(const std::vector<AudioZoneContextInfo>& infos,
                                                std::string& message) {
    static const std::vector<audiomediacommon::AudioUsage> audioUsages{
            audiomediacommon::AudioUsage::UNKNOWN,
            audiomediacommon::AudioUsage::MEDIA,
            audiomediacommon::AudioUsage::VOICE_COMMUNICATION,
            audiomediacommon::AudioUsage::VOICE_COMMUNICATION_SIGNALLING,
            audiomediacommon::AudioUsage::ALARM,
            audiomediacommon::AudioUsage::NOTIFICATION,
            audiomediacommon::AudioUsage::NOTIFICATION_TELEPHONY_RINGTONE,
            audiomediacommon::AudioUsage::NOTIFICATION_EVENT,
            audiomediacommon::AudioUsage::ASSISTANCE_ACCESSIBILITY,
            audiomediacommon::AudioUsage::ASSISTANCE_NAVIGATION_GUIDANCE,
            audiomediacommon::AudioUsage::ASSISTANCE_SONIFICATION,
            audiomediacommon::AudioUsage::GAME,
            audiomediacommon::AudioUsage::ASSISTANT,
            audiomediacommon::AudioUsage::CALL_ASSISTANT,
            audiomediacommon::AudioUsage::EMERGENCY,
            audiomediacommon::AudioUsage::SAFETY,
            audiomediacommon::AudioUsage::VEHICLE_STATUS,
            audiomediacommon::AudioUsage::ANNOUNCEMENT,
    };

    std::set<audiomediacommon::AudioUsage> contextUsages;
    for (const auto& contextInfo : infos) {
        addContextUsages(contextInfo, contextUsages);
    }

    bool allUsagesPresent = true;
    for (const auto& usage : audioUsages) {
        if (contextUsages.contains(usage)) {
            continue;
        }
        if (message.empty()) {
            message = " Missing usage(s): ";
        }
        message += audiomediacommon::toString(usage) + ", ";
        allUsagesPresent = false;
    }
    return allUsagesPresent;
}

bool contextContainsAllAudioAttributeUsages(const AudioZoneContext& context, std::string& message) {
    return contextInfosContainAllAudioAttributeUsages(context.audioContextInfos, message);
}

std::vector<std::string> getContextInfoNamesForAudioRoute(const DeviceToContextEntry& route) {
    std::vector<std::string> contextInfoNames;
    contextInfoNames.reserve(route.contextNames.size());
    for (const auto& contextName : route.contextNames) {
        contextInfoNames.push_back(android::internal::ToString(contextName));
    }
    return contextInfoNames;
}

std::vector<std::string> getContextInfoNamesForVolumeGroup(const VolumeGroupConfig& group) {
    std::vector<std::string> contextInfoNames;
    for (const auto& route : group.carAudioRoutes) {
        std::vector<std::string> routeContexts = getContextInfoNamesForAudioRoute(route);
        contextInfoNames.insert(contextInfoNames.begin(), routeContexts.begin(),
                                routeContexts.end());
    }
    return contextInfoNames;
}

}  // namespace testutils
}  // namespace audiocontrol
}  // namespace hardware
}  // namespace android