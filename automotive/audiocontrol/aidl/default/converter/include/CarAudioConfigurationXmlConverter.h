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

#ifndef MAIN8_CARAUDIOCONFIGURATIONXMLCONVERTER_H
#define MAIN8_CARAUDIOCONFIGURATIONXMLCONVERTER_H

#include <string>

#include "../include/CarAudioConfigurationUtils.h"

#include <aidl/android/hardware/automotive/audiocontrol/AudioDeviceConfiguration.h>
#include <aidl/android/hardware/automotive/audiocontrol/AudioZone.h>
#include <aidl/android/hardware/automotive/audiocontrol/AudioZoneContext.h>

namespace android::hardware::automotive::audiocontrol {
class CarAudioConfigurationType;
}

namespace android {
namespace hardware {
namespace audiocontrol {
namespace internal {

class CarAudioConfigurationXmlConverter {
  public:
    explicit CarAudioConfigurationXmlConverter(const std::string& audioConfigFile,
                                               const std::string& fadeConfigFile)
        : mAudioConfigFile(audioConfigFile), mFadeConfigFile(fadeConfigFile) {
        init();
    }

    ::aidl::android::hardware::automotive::audiocontrol::AudioDeviceConfiguration
    getAudioDeviceConfiguration() const;

    std::vector<::aidl::android::hardware::automotive::audiocontrol::AudioZone> getAudioZones()
            const;
    std::vector<::aidl::android::media::audio::common::AudioPort> getOutputMirroringDevices() const;

    const std::string getErrors() const { return mParseErrors; }

  private:
    void init();
    void initNonDynamicRouting();
    void initFadeConfigurations();
    void initAudioDeviceConfiguration(
            const ::android::hardware::automotive::audiocontrol::CarAudioConfigurationType&
                    carAudioConfigurationType);
    void initCarAudioConfigurations(
            const ::android::hardware::automotive::audiocontrol::CarAudioConfigurationType&
                    carAudioConfigurationType);
    void parseAudioDeviceConfigurations(
            const ::android::hardware::automotive::audiocontrol::CarAudioConfigurationType&
                    carAudioConfigurationType);

    const std::string mAudioConfigFile;
    const std::string mFadeConfigFile;
    ::aidl::android::hardware::automotive::audiocontrol::AudioDeviceConfiguration
            mAudioDeviceConfiguration;
    std::optional<::aidl::android::hardware::automotive::audiocontrol::AudioZoneContext>
            mAudioZoneContext;
    std::vector<::aidl::android::hardware::automotive::audiocontrol::AudioZone> mAudioZones;
    std::vector<::aidl::android::media::audio::common::AudioPort> mOutputMirroringDevices;
    std::string mParseErrors;
    std::unordered_map<std::string,
                       ::aidl::android::hardware::automotive::audiocontrol::AudioFadeConfiguration>
            mFadeConfigurations;
};

}  // namespace internal
}  // namespace audiocontrol
}  // namespace hardware
}  // namespace android

#endif  // MAIN8_CARAUDIOCONFIGURATIONXMLCONVERTER_H
