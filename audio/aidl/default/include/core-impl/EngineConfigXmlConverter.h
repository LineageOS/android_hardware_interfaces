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
#include <unordered_map>

#include <android_audio_policy_engine_configuration.h>
#include <android_audio_policy_engine_configuration_enums.h>
#include <media/AidlConversionUtil.h>

#include "core-impl/XmlConverter.h"

namespace aidl::android::hardware::audio::core::internal {

class EngineConfigXmlConverter {
  public:
    explicit EngineConfigXmlConverter(const std::string& configFilePath)
        : mConverter(configFilePath, &::android::audio::policy::engine::configuration::read) {
        if (mConverter.getXsdcConfig()) {
            init();
        }
    }

    std::string getError() const { return mConverter.getError(); }
    ::android::status_t getStatus() const { return mConverter.getStatus(); }

    ::aidl::android::media::audio::common::AudioHalEngineConfig& getAidlEngineConfig();

  private:
    const std::optional<::android::audio::policy::engine::configuration::Configuration>&
    getXsdcConfig() {
        return mConverter.getXsdcConfig();
    }
    void init();
    void initProductStrategyMap();
    ConversionResult<::aidl::android::media::audio::common::AudioAttributes>
    convertAudioAttributesToAidl(
            const ::android::audio::policy::engine::configuration::AttributesType&
                    xsdcAudioAttributes);
    ConversionResult<::aidl::android::media::audio::common::AudioHalAttributesGroup>
    convertAttributesGroupToAidl(
            const ::android::audio::policy::engine::configuration::AttributesGroup&
                    xsdcAttributesGroup);
    ConversionResult<::aidl::android::media::audio::common::AudioHalProductStrategy>
    convertProductStrategyToAidl(const ::android::audio::policy::engine::configuration::
                                         ProductStrategies::ProductStrategy& xsdcProductStrategy);
    ConversionResult<int> convertProductStrategyNameToAidl(
            const std::string& xsdcProductStrategyName);
    ConversionResult<::aidl::android::media::audio::common::AudioHalVolumeCurve>
    convertVolumeCurveToAidl(
            const ::android::audio::policy::engine::configuration::Volume& xsdcVolumeCurve);
    ConversionResult<::aidl::android::media::audio::common::AudioHalVolumeGroup>
    convertVolumeGroupToAidl(
            const ::android::audio::policy::engine::configuration::VolumeGroupsType::VolumeGroup&
                    xsdcVolumeGroup);

    ::aidl::android::media::audio::common::AudioHalEngineConfig mAidlEngineConfig;
    XmlConverter<::android::audio::policy::engine::configuration::Configuration> mConverter;
    std::unordered_map<std::string,
                       ::android::audio::policy::engine::configuration::AttributesRefType>
            mAttributesReferenceMap;
    std::unordered_map<std::string, ::android::audio::policy::engine::configuration::VolumeRef>
            mVolumesReferenceMap;
    std::unordered_map<std::string, int> mProductStrategyMap;
    int mNextVendorStrategy = ::aidl::android::media::audio::common::AudioHalProductStrategy::
            VENDOR_STRATEGY_ID_START;
    std::optional<int> mDefaultProductStrategyId;
};

}  // namespace aidl::android::hardware::audio::core::internal
