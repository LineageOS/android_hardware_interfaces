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

#include <map>
#include <memory>
#include <vector>

#include <aidl/android/hardware/audio/core/AudioPatch.h>
#include <aidl/android/hardware/audio/core/AudioRoute.h>
#include <aidl/android/media/audio/common/AudioPort.h>
#include <aidl/android/media/audio/common/AudioPortConfig.h>
#include <aidl/android/media/audio/common/MicrophoneInfo.h>

namespace aidl::android::hardware::audio::core::internal {

struct Configuration {
    std::vector<::aidl::android::media::audio::common::MicrophoneInfo> microphones;
    std::vector<::aidl::android::media::audio::common::AudioPort> ports;
    std::vector<::aidl::android::media::audio::common::AudioPortConfig> portConfigs;
    std::vector<::aidl::android::media::audio::common::AudioPortConfig> initialConfigs;
    // Port id -> List of profiles to use when the device port state is set to 'connected'
    // in connection simulation mode.
    std::map<int32_t, std::vector<::aidl::android::media::audio::common::AudioProfile>>
            connectedProfiles;
    std::vector<AudioRoute> routes;
    std::vector<AudioPatch> patches;
    int32_t nextPortId = 1;
    int32_t nextPatchId = 1;
};

std::unique_ptr<Configuration> getPrimaryConfiguration();
std::unique_ptr<Configuration> getRSubmixConfiguration();
std::unique_ptr<Configuration> getStubConfiguration();
std::unique_ptr<Configuration> getUsbConfiguration();
std::unique_ptr<Configuration> getBluetoothConfiguration();

}  // namespace aidl::android::hardware::audio::core::internal
