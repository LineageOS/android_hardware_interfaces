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

#include <fcntl.h>
#include <inttypes.h>
#include <unistd.h>

#include <functional>
#include <unordered_map>

#define LOG_TAG "AHAL_ApmXmlConverter"
#include <android-base/logging.h>

#include <aidl/android/media/audio/common/AudioHalEngineConfig.h>
#include <media/stagefright/foundation/MediaDefs.h>
#include <system/audio-base-utils.h>

#include "core-impl/AidlConversionXsdc.h"
#include "core-impl/AudioPolicyConfigXmlConverter.h"
#include "core-impl/XsdcConversion.h"

using aidl::android::media::audio::common::AudioFormatDescription;
using aidl::android::media::audio::common::AudioHalEngineConfig;
using aidl::android::media::audio::common::AudioHalVolumeCurve;
using aidl::android::media::audio::common::AudioHalVolumeGroup;
using aidl::android::media::audio::common::AudioStreamType;

namespace ap_xsd = android::audio::policy::configuration;

namespace aidl::android::hardware::audio::core::internal {

static const int kDefaultVolumeIndexMin = 0;
static const int kDefaultVolumeIndexMax = 100;
static const int KVolumeIndexDeferredToAudioService = -1;

ConversionResult<AudioHalVolumeCurve> AudioPolicyConfigXmlConverter::convertVolumeCurveToAidl(
        const ap_xsd::Volume& xsdcVolumeCurve) {
    AudioHalVolumeCurve aidlVolumeCurve;
    aidlVolumeCurve.deviceCategory =
            static_cast<AudioHalVolumeCurve::DeviceCategory>(xsdcVolumeCurve.getDeviceCategory());
    if (xsdcVolumeCurve.hasRef()) {
        if (mVolumesReferenceMap.empty()) {
            mVolumesReferenceMap = generateReferenceMap<ap_xsd::Volumes, ap_xsd::Reference>(
                    getXsdcConfig()->getVolumes());
        }
        aidlVolumeCurve.curvePoints = VALUE_OR_FATAL(
                (convertCollectionToAidl<std::string, AudioHalVolumeCurve::CurvePoint>(
                        mVolumesReferenceMap.at(xsdcVolumeCurve.getRef()).getPoint(),
                        &convertCurvePointToAidl)));
    } else {
        aidlVolumeCurve.curvePoints = VALUE_OR_FATAL(
                (convertCollectionToAidl<std::string, AudioHalVolumeCurve::CurvePoint>(
                        xsdcVolumeCurve.getPoint(), &convertCurvePointToAidl)));
    }
    return aidlVolumeCurve;
}

void AudioPolicyConfigXmlConverter::mapStreamToVolumeCurve(const ap_xsd::Volume& xsdcVolumeCurve) {
    mStreamToVolumeCurvesMap[xsdcVolumeCurve.getStream()].push_back(
            VALUE_OR_FATAL(convertVolumeCurveToAidl(xsdcVolumeCurve)));
}

const SurroundSoundConfig& AudioPolicyConfigXmlConverter::getSurroundSoundConfig() {
    static const SurroundSoundConfig aidlSurroundSoundConfig = [this]() {
        if (auto xsdcConfig = getXsdcConfig(); xsdcConfig && xsdcConfig->hasSurroundSound()) {
            auto configConv = xsdc2aidl_SurroundSoundConfig(*xsdcConfig->getFirstSurroundSound());
            if (configConv.ok()) {
                return configConv.value();
            }
            LOG(ERROR) << "There was an error converting surround formats to AIDL: "
                       << configConv.error();
        }
        LOG(WARNING) << "Audio policy config does not have <surroundSound> section, using default";
        return getDefaultSurroundSoundConfig();
    }();
    return aidlSurroundSoundConfig;
}

std::unique_ptr<AudioPolicyConfigXmlConverter::ModuleConfigs>
AudioPolicyConfigXmlConverter::releaseModuleConfigs() {
    return std::move(mModuleConfigurations);
}

const AudioHalEngineConfig& AudioPolicyConfigXmlConverter::getAidlEngineConfig() {
    if (mAidlEngineConfig.volumeGroups.empty() && getXsdcConfig() &&
        getXsdcConfig()->hasVolumes()) {
        parseVolumes();
    }
    return mAidlEngineConfig;
}

// static
const SurroundSoundConfig& AudioPolicyConfigXmlConverter::getDefaultSurroundSoundConfig() {
    // Provide a config similar to the one used by the framework by default
    // (see AudioPolicyConfig::setDefaultSurroundFormats).
#define ENCODED_FORMAT(format)        \
    AudioFormatDescription {          \
        .encoding = ::android::format \
    }
#define SIMPLE_FORMAT(format)                   \
    SurroundSoundConfig::SurroundFormatFamily { \
        .primaryFormat = ENCODED_FORMAT(format) \
    }

    static const SurroundSoundConfig defaultConfig = {
            .formatFamilies = {
                    SIMPLE_FORMAT(MEDIA_MIMETYPE_AUDIO_AC3),
                    SIMPLE_FORMAT(MEDIA_MIMETYPE_AUDIO_EAC3),
                    SIMPLE_FORMAT(MEDIA_MIMETYPE_AUDIO_DTS),
                    SIMPLE_FORMAT(MEDIA_MIMETYPE_AUDIO_DTS_HD),
                    SIMPLE_FORMAT(MEDIA_MIMETYPE_AUDIO_DTS_HD_MA),
                    SIMPLE_FORMAT(MEDIA_MIMETYPE_AUDIO_DTS_UHD_P1),
                    SIMPLE_FORMAT(MEDIA_MIMETYPE_AUDIO_DTS_UHD_P2),
                    SIMPLE_FORMAT(MEDIA_MIMETYPE_AUDIO_DOLBY_TRUEHD),
                    SIMPLE_FORMAT(MEDIA_MIMETYPE_AUDIO_EAC3_JOC),
                    SurroundSoundConfig::SurroundFormatFamily{
                            .primaryFormat = ENCODED_FORMAT(MEDIA_MIMETYPE_AUDIO_AAC_LC),
                            .subFormats =
                                    {
                                            ENCODED_FORMAT(MEDIA_MIMETYPE_AUDIO_AAC_HE_V1),
                                            ENCODED_FORMAT(MEDIA_MIMETYPE_AUDIO_AAC_HE_V2),
                                            ENCODED_FORMAT(MEDIA_MIMETYPE_AUDIO_AAC_ELD),
                                            ENCODED_FORMAT(MEDIA_MIMETYPE_AUDIO_AAC_XHE),
                                    }},
                    SIMPLE_FORMAT(MEDIA_MIMETYPE_AUDIO_AC4),
            }};
#undef SIMPLE_FORMAT
#undef ENCODED_FORMAT

    return defaultConfig;
}

void AudioPolicyConfigXmlConverter::mapStreamsToVolumeCurves() {
    if (getXsdcConfig()->hasVolumes()) {
        for (const ap_xsd::Volumes& xsdcWrapperType : getXsdcConfig()->getVolumes()) {
            for (const ap_xsd::Volume& xsdcVolume : xsdcWrapperType.getVolume()) {
                mapStreamToVolumeCurve(xsdcVolume);
            }
        }
    }
}

void AudioPolicyConfigXmlConverter::addVolumeGroupstoEngineConfig() {
    for (const auto& [xsdcStream, volumeCurves] : mStreamToVolumeCurvesMap) {
        AudioHalVolumeGroup volumeGroup;
        volumeGroup.name = ap_xsd::toString(xsdcStream);
        if (static_cast<int>(xsdcStream) >= AUDIO_STREAM_PUBLIC_CNT) {
            volumeGroup.minIndex = kDefaultVolumeIndexMin;
            volumeGroup.maxIndex = kDefaultVolumeIndexMax;
        } else {
            volumeGroup.minIndex = KVolumeIndexDeferredToAudioService;
            volumeGroup.maxIndex = KVolumeIndexDeferredToAudioService;
        }
        volumeGroup.volumeCurves = volumeCurves;
        mAidlEngineConfig.volumeGroups.push_back(std::move(volumeGroup));
    }
}

void AudioPolicyConfigXmlConverter::parseVolumes() {
    if (mStreamToVolumeCurvesMap.empty() && getXsdcConfig()->hasVolumes()) {
        mapStreamsToVolumeCurves();
        addVolumeGroupstoEngineConfig();
    }
}

void AudioPolicyConfigXmlConverter::init() {
    if (!getXsdcConfig()->hasModules()) return;
    for (const ap_xsd::Modules& xsdcModulesType : getXsdcConfig()->getModules()) {
        if (!xsdcModulesType.has_module()) continue;
        for (const ap_xsd::Modules::Module& xsdcModule : xsdcModulesType.get_module()) {
            // 'primary' in the XML schema used by HIDL is equivalent to 'default' module.
            const std::string name =
                    xsdcModule.getName() != "primary" ? xsdcModule.getName() : "default";
            if (name != "r_submix") {
                mModuleConfigurations->emplace_back(
                        name, VALUE_OR_FATAL(convertModuleConfigToAidl(xsdcModule)));
            } else {
                // See the note on the 'getRSubmixConfiguration' function.
                mModuleConfigurations->emplace_back(name, nullptr);
            }
        }
    }
}

}  // namespace aidl::android::hardware::audio::core::internal
