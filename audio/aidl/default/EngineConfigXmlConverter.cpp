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

#define LOG_TAG "AHAL_Config"
#include <aidl/android/media/audio/common/AudioFlag.h>
#include <aidl/android/media/audio/common/AudioHalEngineConfig.h>
#include <aidl/android/media/audio/common/AudioProductStrategyType.h>
#include <android-base/logging.h>

#include "core-impl/EngineConfigXmlConverter.h"
#include "core-impl/XsdcConversion.h"

using aidl::android::media::audio::common::AudioAttributes;
using aidl::android::media::audio::common::AudioContentType;
using aidl::android::media::audio::common::AudioFlag;
using aidl::android::media::audio::common::AudioHalAttributesGroup;
using aidl::android::media::audio::common::AudioHalCapCriterion;
using aidl::android::media::audio::common::AudioHalCapCriterionType;
using aidl::android::media::audio::common::AudioHalEngineConfig;
using aidl::android::media::audio::common::AudioHalProductStrategy;
using aidl::android::media::audio::common::AudioHalVolumeCurve;
using aidl::android::media::audio::common::AudioHalVolumeGroup;
using aidl::android::media::audio::common::AudioProductStrategyType;
using aidl::android::media::audio::common::AudioSource;
using aidl::android::media::audio::common::AudioStreamType;
using aidl::android::media::audio::common::AudioUsage;
using ::android::BAD_VALUE;
using ::android::base::unexpected;

namespace eng_xsd = android::audio::policy::engine::configuration;

namespace aidl::android::hardware::audio::core::internal {

void EngineConfigXmlConverter::initProductStrategyMap() {
#define STRATEGY_ENTRY(name) {"STRATEGY_" #name, static_cast<int>(AudioProductStrategyType::name)}

    mProductStrategyMap = {STRATEGY_ENTRY(MEDIA),
                           STRATEGY_ENTRY(PHONE),
                           STRATEGY_ENTRY(SONIFICATION),
                           STRATEGY_ENTRY(SONIFICATION_RESPECTFUL),
                           STRATEGY_ENTRY(DTMF),
                           STRATEGY_ENTRY(ENFORCED_AUDIBLE),
                           STRATEGY_ENTRY(TRANSMITTED_THROUGH_SPEAKER),
                           STRATEGY_ENTRY(ACCESSIBILITY)};
#undef STRATEGY_ENTRY
}

ConversionResult<int> EngineConfigXmlConverter::convertProductStrategyNameToAidl(
        const std::string& xsdcProductStrategyName) {
    const auto [it, success] = mProductStrategyMap.insert(
            std::make_pair(xsdcProductStrategyName, mNextVendorStrategy));
    if (success) {
        mNextVendorStrategy++;
    }
    return it->second;
}

bool isDefaultAudioAttributes(const AudioAttributes& attributes) {
    return ((attributes.contentType == AudioContentType::UNKNOWN) &&
            (attributes.usage == AudioUsage::UNKNOWN) &&
            (attributes.source == AudioSource::DEFAULT) && (attributes.flags == 0) &&
            (attributes.tags.empty()));
}

ConversionResult<AudioAttributes> EngineConfigXmlConverter::convertAudioAttributesToAidl(
        const eng_xsd::AttributesType& xsdcAudioAttributes) {
    if (xsdcAudioAttributes.hasAttributesRef()) {
        if (mAttributesReferenceMap.empty()) {
            mAttributesReferenceMap =
                    generateReferenceMap<eng_xsd::AttributesRef, eng_xsd::AttributesRefType>(
                            getXsdcConfig()->getAttributesRef());
        }
        return convertAudioAttributesToAidl(
                *(mAttributesReferenceMap.at(xsdcAudioAttributes.getAttributesRef())
                          .getFirstAttributes()));
    }
    AudioAttributes aidlAudioAttributes;
    if (xsdcAudioAttributes.hasContentType()) {
        aidlAudioAttributes.contentType = static_cast<AudioContentType>(
                xsdcAudioAttributes.getFirstContentType()->getValue());
    }
    if (xsdcAudioAttributes.hasUsage()) {
        aidlAudioAttributes.usage =
                static_cast<AudioUsage>(xsdcAudioAttributes.getFirstUsage()->getValue());
    }
    if (xsdcAudioAttributes.hasSource()) {
        aidlAudioAttributes.source =
                static_cast<AudioSource>(xsdcAudioAttributes.getFirstSource()->getValue());
    }
    if (xsdcAudioAttributes.hasFlags()) {
        std::vector<eng_xsd::FlagType> xsdcFlagTypeVec =
                xsdcAudioAttributes.getFirstFlags()->getValue();
        for (const eng_xsd::FlagType& xsdcFlagType : xsdcFlagTypeVec) {
            if (xsdcFlagType != eng_xsd::FlagType::AUDIO_FLAG_NONE) {
                aidlAudioAttributes.flags |= 1 << (static_cast<int>(xsdcFlagType) - 1);
            }
        }
    }
    if (xsdcAudioAttributes.hasBundle()) {
        const eng_xsd::BundleType* xsdcBundle = xsdcAudioAttributes.getFirstBundle();
        aidlAudioAttributes.tags[0] = xsdcBundle->getKey() + "=" + xsdcBundle->getValue();
    }
    if (isDefaultAudioAttributes(aidlAudioAttributes)) {
        mDefaultProductStrategyId = std::optional<int>{-1};
    }
    return aidlAudioAttributes;
}

ConversionResult<AudioHalAttributesGroup> EngineConfigXmlConverter::convertAttributesGroupToAidl(
        const eng_xsd::AttributesGroup& xsdcAttributesGroup) {
    AudioHalAttributesGroup aidlAttributesGroup;
    static const int kStreamTypeEnumOffset =
            static_cast<int>(eng_xsd::Stream::AUDIO_STREAM_VOICE_CALL) -
            static_cast<int>(AudioStreamType::VOICE_CALL);
    aidlAttributesGroup.streamType = static_cast<AudioStreamType>(
            static_cast<int>(xsdcAttributesGroup.getStreamType()) - kStreamTypeEnumOffset);
    aidlAttributesGroup.volumeGroupName = xsdcAttributesGroup.getVolumeGroup();
    if (xsdcAttributesGroup.hasAttributes_optional()) {
        aidlAttributesGroup.attributes =
                VALUE_OR_FATAL((convertCollectionToAidl<eng_xsd::AttributesType, AudioAttributes>(
                        xsdcAttributesGroup.getAttributes_optional(),
                        std::bind(&EngineConfigXmlConverter::convertAudioAttributesToAidl, this,
                                  std::placeholders::_1))));
    } else if (xsdcAttributesGroup.hasContentType_optional() ||
               xsdcAttributesGroup.hasUsage_optional() ||
               xsdcAttributesGroup.hasSource_optional() ||
               xsdcAttributesGroup.hasFlags_optional() ||
               xsdcAttributesGroup.hasBundle_optional()) {
        aidlAttributesGroup.attributes.push_back(VALUE_OR_FATAL(convertAudioAttributesToAidl(
                eng_xsd::AttributesType(xsdcAttributesGroup.getContentType_optional(),
                                        xsdcAttributesGroup.getUsage_optional(),
                                        xsdcAttributesGroup.getSource_optional(),
                                        xsdcAttributesGroup.getFlags_optional(),
                                        xsdcAttributesGroup.getBundle_optional(), std::nullopt))));

    } else {
        LOG(ERROR) << __func__ << " Review Audio Policy config: no audio attributes provided for "
                   << aidlAttributesGroup.toString();
        return unexpected(BAD_VALUE);
    }
    return aidlAttributesGroup;
}

ConversionResult<AudioHalProductStrategy> EngineConfigXmlConverter::convertProductStrategyToAidl(
        const eng_xsd::ProductStrategies::ProductStrategy& xsdcProductStrategy) {
    AudioHalProductStrategy aidlProductStrategy;

    aidlProductStrategy.id =
            VALUE_OR_FATAL(convertProductStrategyNameToAidl(xsdcProductStrategy.getName()));

    if (xsdcProductStrategy.hasAttributesGroup()) {
        aidlProductStrategy.attributesGroups = VALUE_OR_FATAL(
                (convertCollectionToAidl<eng_xsd::AttributesGroup, AudioHalAttributesGroup>(
                        xsdcProductStrategy.getAttributesGroup(),
                        std::bind(&EngineConfigXmlConverter::convertAttributesGroupToAidl, this,
                                  std::placeholders::_1))));
    }
    if ((mDefaultProductStrategyId != std::nullopt) && (mDefaultProductStrategyId.value() == -1)) {
        mDefaultProductStrategyId = aidlProductStrategy.id;
    }
    return aidlProductStrategy;
}

ConversionResult<AudioHalVolumeCurve> EngineConfigXmlConverter::convertVolumeCurveToAidl(
        const eng_xsd::Volume& xsdcVolumeCurve) {
    AudioHalVolumeCurve aidlVolumeCurve;
    aidlVolumeCurve.deviceCategory =
            static_cast<AudioHalVolumeCurve::DeviceCategory>(xsdcVolumeCurve.getDeviceCategory());
    if (xsdcVolumeCurve.hasRef()) {
        if (mVolumesReferenceMap.empty()) {
            mVolumesReferenceMap = generateReferenceMap<eng_xsd::VolumesType, eng_xsd::VolumeRef>(
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

ConversionResult<AudioHalVolumeGroup> EngineConfigXmlConverter::convertVolumeGroupToAidl(
        const eng_xsd::VolumeGroupsType::VolumeGroup& xsdcVolumeGroup) {
    AudioHalVolumeGroup aidlVolumeGroup;
    aidlVolumeGroup.name = xsdcVolumeGroup.getName();
    aidlVolumeGroup.minIndex = xsdcVolumeGroup.getIndexMin();
    aidlVolumeGroup.maxIndex = xsdcVolumeGroup.getIndexMax();
    aidlVolumeGroup.volumeCurves =
            VALUE_OR_FATAL((convertCollectionToAidl<eng_xsd::Volume, AudioHalVolumeCurve>(
                    xsdcVolumeGroup.getVolume(),
                    std::bind(&EngineConfigXmlConverter::convertVolumeCurveToAidl, this,
                              std::placeholders::_1))));
    return aidlVolumeGroup;
}

AudioHalEngineConfig& EngineConfigXmlConverter::getAidlEngineConfig() {
    return mAidlEngineConfig;
}

void EngineConfigXmlConverter::init() {
    initProductStrategyMap();
    if (getXsdcConfig()->hasProductStrategies()) {
        mAidlEngineConfig.productStrategies = VALUE_OR_FATAL(
                (convertWrappedCollectionToAidl<eng_xsd::ProductStrategies,
                                                eng_xsd::ProductStrategies::ProductStrategy,
                                                AudioHalProductStrategy>(
                        getXsdcConfig()->getProductStrategies(),
                        &eng_xsd::ProductStrategies::getProductStrategy,
                        std::bind(&EngineConfigXmlConverter::convertProductStrategyToAidl, this,
                                  std::placeholders::_1))));
        if (mDefaultProductStrategyId) {
            mAidlEngineConfig.defaultProductStrategyId = mDefaultProductStrategyId.value();
        }
    }
    if (getXsdcConfig()->hasVolumeGroups()) {
        mAidlEngineConfig.volumeGroups = VALUE_OR_FATAL(
                (convertWrappedCollectionToAidl<eng_xsd::VolumeGroupsType,
                                                eng_xsd::VolumeGroupsType::VolumeGroup,
                                                AudioHalVolumeGroup>(
                        getXsdcConfig()->getVolumeGroups(),
                        &eng_xsd::VolumeGroupsType::getVolumeGroup,
                        std::bind(&EngineConfigXmlConverter::convertVolumeGroupToAidl, this,
                                  std::placeholders::_1))));
    }
    if (getXsdcConfig()->hasCriteria() && getXsdcConfig()->hasCriterion_types()) {
        AudioHalEngineConfig::CapSpecificConfig capSpecificConfig;
        capSpecificConfig.criteria = VALUE_OR_FATAL(
                (convertWrappedCollectionToAidl<eng_xsd::CriteriaType, eng_xsd::CriterionType,
                                                AudioHalCapCriterion>(
                        getXsdcConfig()->getCriteria(), &eng_xsd::CriteriaType::getCriterion,
                        &convertCapCriterionToAidl)));
        capSpecificConfig.criterionTypes =
                VALUE_OR_FATAL((convertWrappedCollectionToAidl<eng_xsd::CriterionTypesType,
                                                               eng_xsd::CriterionTypeType,
                                                               AudioHalCapCriterionType>(
                        getXsdcConfig()->getCriterion_types(),
                        &eng_xsd::CriterionTypesType::getCriterion_type,
                        &convertCapCriterionTypeToAidl)));
    }
}
}  // namespace aidl::android::hardware::audio::core::internal
