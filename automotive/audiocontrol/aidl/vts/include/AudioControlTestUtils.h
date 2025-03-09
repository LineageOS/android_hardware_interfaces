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

#ifndef MAIN8_AUDIOCONTROLTESTUTILS_H
#define MAIN8_AUDIOCONTROLTESTUTILS_H

#include <android/hardware/automotive/audiocontrol/IAudioControl.h>
#include <string>
#include <vector>

namespace android {
namespace hardware {
namespace audiocontrol {
namespace testutils {

std::string toAlphaNumeric(const std::string& info);

bool getAudioPortDeviceDescriptor(
        const android::media::audio::common::AudioPort& audioPort,
        android::media::audio::common::AudioDeviceDescription& description);

bool getAddressForAudioPort(const android::media::audio::common::AudioPort& audioPort,
                            std::string& address);

bool getAddressForAudioDevice(
        const android::hardware::automotive::audiocontrol::DeviceToContextEntry& device,
        std::string& address);

std::vector<std::string> getDeviceAddressesForVolumeGroup(
        const android::hardware::automotive::audiocontrol::VolumeGroupConfig& config);

std::vector<std::string> getDeviceAddressesForZoneConfig(
        const android::hardware::automotive::audiocontrol::AudioZoneConfig& config);

std::vector<std::string> getDeviceAddressesForZone(
        const android::hardware::automotive::audiocontrol::AudioZone& config);

bool contextInfosContainAllAudioAttributeUsages(
        const std::vector<android::hardware::automotive::audiocontrol::AudioZoneContextInfo>& infos,
        std::string& message);

bool contextContainsAllAudioAttributeUsages(
        const android::hardware::automotive::audiocontrol::AudioZoneContext& context,
        std::string& message);

std::vector<std::string> getContextInfoNamesForVolumeGroup(
        const android::hardware::automotive::audiocontrol::VolumeGroupConfig& group);

}  // namespace testutils
}  // namespace audiocontrol
}  // namespace hardware
}  // namespace android

#endif  // MAIN8_AUDIOCONTROLTESTUTILS_H
