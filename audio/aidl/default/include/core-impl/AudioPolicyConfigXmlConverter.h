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

#include <string>

#include <aidl/android/hardware/audio/core/SurroundSoundConfig.h>
#include <aidl/android/media/audio/common/AudioHalEngineConfig.h>
#include <android_audio_policy_configuration.h>
#include <android_audio_policy_configuration_enums.h>

#include "core-impl/XmlConverter.h"

namespace aidl::android::hardware::audio::core::internal {

class AudioPolicyConfigXmlConverter {
  public:
    explicit AudioPolicyConfigXmlConverter(const std::string& configFilePath)
        : mConverter(configFilePath, &::android::audio::policy::configuration::read) {}

    std::string getError() const { return mConverter.getError(); }
    ::android::status_t getStatus() const { return mConverter.getStatus(); }

    const ::aidl::android::media::audio::common::AudioHalEngineConfig& getAidlEngineConfig();
    const SurroundSoundConfig& getSurroundSoundConfig();

    // Public for testing purposes.
    static const SurroundSoundConfig& getDefaultSurroundSoundConfig();

  private:
    const std::optional<::android::audio::policy::configuration::AudioPolicyConfiguration>&
    getXsdcConfig() const {
        return mConverter.getXsdcConfig();
    }
    void addVolumeGroupstoEngineConfig();
    void mapStreamToVolumeCurve(
            const ::android::audio::policy::configuration::Volume& xsdcVolumeCurve);
    void mapStreamsToVolumeCurves();
    void parseVolumes();
    ::aidl::android::media::audio::common::AudioHalVolumeCurve::CurvePoint convertCurvePointToAidl(
            const std::string& xsdcCurvePoint);
    ::aidl::android::media::audio::common::AudioHalVolumeCurve convertVolumeCurveToAidl(
            const ::android::audio::policy::configuration::Volume& xsdcVolumeCurve);

    ::aidl::android::media::audio::common::AudioHalEngineConfig mAidlEngineConfig;
    XmlConverter<::android::audio::policy::configuration::AudioPolicyConfiguration> mConverter;
    std::unordered_map<std::string, ::android::audio::policy::configuration::Reference>
            mVolumesReferenceMap;
    std::unordered_map<::android::audio::policy::configuration::AudioStreamType,
                       std::vector<::aidl::android::media::audio::common::AudioHalVolumeCurve>>
            mStreamToVolumeCurvesMap;
};

}  // namespace aidl::android::hardware::audio::core::internal
