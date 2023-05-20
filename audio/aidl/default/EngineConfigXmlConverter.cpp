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

#include <aidl/android/media/audio/common/AudioFlag.h>
#include <aidl/android/media/audio/common/AudioHalEngineConfig.h>
#include <aidl/android/media/audio/common/AudioProductStrategyType.h>

#include "core-impl/EngineConfigXmlConverter.h"

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

namespace xsd = android::audio::policy::engine::configuration;

namespace aidl::android::hardware::audio::core::internal {

/**
 * Valid curve points take the form "<index>,<attenuationMb>", where the index
 * must be in the range [0,100]. kInvalidCurvePointIndex is used to indicate
 * that a point was formatted incorrectly (e.g. if a vendor accidentally typed a
 * '.' instead of a ',' in their XML)-- using such a curve point will result in
 * failed VTS tests.
 */
static const int8_t kInvalidCurvePointIndex = -1;

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

int EngineConfigXmlConverter::convertProductStrategyNameToAidl(
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

AudioAttributes EngineConfigXmlConverter::convertAudioAttributesToAidl(
        const xsd::AttributesType& xsdcAudioAttributes) {
    if (xsdcAudioAttributes.hasAttributesRef()) {
        if (mAttributesReferenceMap.empty()) {
            mAttributesReferenceMap =
                    generateReferenceMap<xsd::AttributesRef, xsd::AttributesRefType>(
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
        std::vector<xsd::FlagType> xsdcFlagTypeVec =
                xsdcAudioAttributes.getFirstFlags()->getValue();
        for (const xsd::FlagType& xsdcFlagType : xsdcFlagTypeVec) {
            if (xsdcFlagType != xsd::FlagType::AUDIO_FLAG_NONE) {
                aidlAudioAttributes.flags |= 1 << (static_cast<int>(xsdcFlagType) - 1);
            }
        }
    }
    if (xsdcAudioAttributes.hasBundle()) {
        const xsd::BundleType* xsdcBundle = xsdcAudioAttributes.getFirstBundle();
        aidlAudioAttributes.tags[0] = xsdcBundle->getKey() + "=" + xsdcBundle->getValue();
    }
    if (isDefaultAudioAttributes(aidlAudioAttributes)) {
        mDefaultProductStrategyId = std::optional<int>{-1};
    }
    return aidlAudioAttributes;
}

AudioHalAttributesGroup EngineConfigXmlConverter::convertAttributesGroupToAidl(
        const xsd::AttributesGroup& xsdcAttributesGroup) {
    AudioHalAttributesGroup aidlAttributesGroup;
    static const int kStreamTypeEnumOffset =
            static_cast<int>(xsd::Stream::AUDIO_STREAM_VOICE_CALL) -
            static_cast<int>(AudioStreamType::VOICE_CALL);
    aidlAttributesGroup.streamType = static_cast<AudioStreamType>(
            static_cast<int>(xsdcAttributesGroup.getStreamType()) - kStreamTypeEnumOffset);
    aidlAttributesGroup.volumeGroupName = xsdcAttributesGroup.getVolumeGroup();
    if (xsdcAttributesGroup.hasAttributes_optional()) {
        aidlAttributesGroup.attributes =
                convertCollectionToAidlUnchecked<xsd::AttributesType, AudioAttributes>(
                        xsdcAttributesGroup.getAttributes_optional(),
                        std::bind(&EngineConfigXmlConverter::convertAudioAttributesToAidl, this,
                                  std::placeholders::_1));
    } else if (xsdcAttributesGroup.hasContentType_optional() ||
               xsdcAttributesGroup.hasUsage_optional() ||
               xsdcAttributesGroup.hasSource_optional() ||
               xsdcAttributesGroup.hasFlags_optional() ||
               xsdcAttributesGroup.hasBundle_optional()) {
        aidlAttributesGroup.attributes.push_back(convertAudioAttributesToAidl(xsd::AttributesType(
                xsdcAttributesGroup.getContentType_optional(),
                xsdcAttributesGroup.getUsage_optional(), xsdcAttributesGroup.getSource_optional(),
                xsdcAttributesGroup.getFlags_optional(), xsdcAttributesGroup.getBundle_optional(),
                std::nullopt)));

    } else {
        // do nothing;
        // TODO: check if this is valid or if we should treat as an error.
        // Currently, attributes are not mandatory in schema, but an AttributesGroup
        // without attributes does not make much sense.
    }
    return aidlAttributesGroup;
}

AudioHalProductStrategy EngineConfigXmlConverter::convertProductStrategyToAidl(
        const xsd::ProductStrategies::ProductStrategy& xsdcProductStrategy) {
    AudioHalProductStrategy aidlProductStrategy;

    aidlProductStrategy.id = convertProductStrategyNameToAidl(xsdcProductStrategy.getName());

    if (xsdcProductStrategy.hasAttributesGroup()) {
        aidlProductStrategy.attributesGroups =
                convertCollectionToAidlUnchecked<xsd::AttributesGroup, AudioHalAttributesGroup>(
                        xsdcProductStrategy.getAttributesGroup(),
                        std::bind(&EngineConfigXmlConverter::convertAttributesGroupToAidl, this,
                                  std::placeholders::_1));
    }
    if ((mDefaultProductStrategyId != std::nullopt) && (mDefaultProductStrategyId.value() == -1)) {
        mDefaultProductStrategyId = aidlProductStrategy.id;
    }
    return aidlProductStrategy;
}

AudioHalVolumeCurve::CurvePoint EngineConfigXmlConverter::convertCurvePointToAidl(
        const std::string& xsdcCurvePoint) {
    AudioHalVolumeCurve::CurvePoint aidlCurvePoint{};
    if (sscanf(xsdcCurvePoint.c_str(), "%" SCNd8 ",%d", &aidlCurvePoint.index,
               &aidlCurvePoint.attenuationMb) != 2) {
        aidlCurvePoint.index = kInvalidCurvePointIndex;
    }
    return aidlCurvePoint;
}

AudioHalVolumeCurve EngineConfigXmlConverter::convertVolumeCurveToAidl(
        const xsd::Volume& xsdcVolumeCurve) {
    AudioHalVolumeCurve aidlVolumeCurve;
    aidlVolumeCurve.deviceCategory =
            static_cast<AudioHalVolumeCurve::DeviceCategory>(xsdcVolumeCurve.getDeviceCategory());
    if (xsdcVolumeCurve.hasRef()) {
        if (mVolumesReferenceMap.empty()) {
            mVolumesReferenceMap = generateReferenceMap<xsd::VolumesType, xsd::VolumeRef>(
                    getXsdcConfig()->getVolumes());
        }
        aidlVolumeCurve.curvePoints =
                convertCollectionToAidlUnchecked<std::string, AudioHalVolumeCurve::CurvePoint>(
                        mVolumesReferenceMap.at(xsdcVolumeCurve.getRef()).getPoint(),
                        std::bind(&EngineConfigXmlConverter::convertCurvePointToAidl, this,
                                  std::placeholders::_1));
    } else {
        aidlVolumeCurve.curvePoints =
                convertCollectionToAidlUnchecked<std::string, AudioHalVolumeCurve::CurvePoint>(
                        xsdcVolumeCurve.getPoint(),
                        std::bind(&EngineConfigXmlConverter::convertCurvePointToAidl, this,
                                  std::placeholders::_1));
    }
    return aidlVolumeCurve;
}

AudioHalVolumeGroup EngineConfigXmlConverter::convertVolumeGroupToAidl(
        const xsd::VolumeGroupsType::VolumeGroup& xsdcVolumeGroup) {
    AudioHalVolumeGroup aidlVolumeGroup;
    aidlVolumeGroup.name = xsdcVolumeGroup.getName();
    aidlVolumeGroup.minIndex = xsdcVolumeGroup.getIndexMin();
    aidlVolumeGroup.maxIndex = xsdcVolumeGroup.getIndexMax();
    aidlVolumeGroup.volumeCurves =
            convertCollectionToAidlUnchecked<xsd::Volume, AudioHalVolumeCurve>(
                    xsdcVolumeGroup.getVolume(),
                    std::bind(&EngineConfigXmlConverter::convertVolumeCurveToAidl, this,
                              std::placeholders::_1));
    return aidlVolumeGroup;
}

AudioHalCapCriterion EngineConfigXmlConverter::convertCapCriterionToAidl(
        const xsd::CriterionType& xsdcCriterion) {
    AudioHalCapCriterion aidlCapCriterion;
    aidlCapCriterion.name = xsdcCriterion.getName();
    aidlCapCriterion.criterionTypeName = xsdcCriterion.getType();
    aidlCapCriterion.defaultLiteralValue = xsdcCriterion.get_default();
    return aidlCapCriterion;
}

std::string EngineConfigXmlConverter::convertCriterionTypeValueToAidl(
        const xsd::ValueType& xsdcCriterionTypeValue) {
    return xsdcCriterionTypeValue.getLiteral();
}

AudioHalCapCriterionType EngineConfigXmlConverter::convertCapCriterionTypeToAidl(
        const xsd::CriterionTypeType& xsdcCriterionType) {
    AudioHalCapCriterionType aidlCapCriterionType;
    aidlCapCriterionType.name = xsdcCriterionType.getName();
    aidlCapCriterionType.isInclusive = !(static_cast<bool>(xsdcCriterionType.getType()));
    aidlCapCriterionType.values =
            convertWrappedCollectionToAidlUnchecked<xsd::ValuesType, xsd::ValueType, std::string>(
                    xsdcCriterionType.getValues(), &xsd::ValuesType::getValue,
                    std::bind(&EngineConfigXmlConverter::convertCriterionTypeValueToAidl, this,
                              std::placeholders::_1));
    return aidlCapCriterionType;
}

AudioHalEngineConfig& EngineConfigXmlConverter::getAidlEngineConfig() {
    return mAidlEngineConfig;
}

void EngineConfigXmlConverter::init() {
    initProductStrategyMap();
    if (getXsdcConfig()->hasProductStrategies()) {
        mAidlEngineConfig.productStrategies =
                convertWrappedCollectionToAidlUnchecked<xsd::ProductStrategies,
                                                        xsd::ProductStrategies::ProductStrategy,
                                                        AudioHalProductStrategy>(
                        getXsdcConfig()->getProductStrategies(),
                        &xsd::ProductStrategies::getProductStrategy,
                        std::bind(&EngineConfigXmlConverter::convertProductStrategyToAidl, this,
                                  std::placeholders::_1));
        if (mDefaultProductStrategyId) {
            mAidlEngineConfig.defaultProductStrategyId = mDefaultProductStrategyId.value();
        }
    }
    if (getXsdcConfig()->hasVolumeGroups()) {
        mAidlEngineConfig.volumeGroups = convertWrappedCollectionToAidlUnchecked<
                xsd::VolumeGroupsType, xsd::VolumeGroupsType::VolumeGroup, AudioHalVolumeGroup>(
                getXsdcConfig()->getVolumeGroups(), &xsd::VolumeGroupsType::getVolumeGroup,
                std::bind(&EngineConfigXmlConverter::convertVolumeGroupToAidl, this,
                          std::placeholders::_1));
    }
    if (getXsdcConfig()->hasCriteria() && getXsdcConfig()->hasCriterion_types()) {
        AudioHalEngineConfig::CapSpecificConfig capSpecificConfig;
        capSpecificConfig.criteria =
                convertWrappedCollectionToAidlUnchecked<xsd::CriteriaType, xsd::CriterionType,
                                                        AudioHalCapCriterion>(
                        getXsdcConfig()->getCriteria(), &xsd::CriteriaType::getCriterion,
                        std::bind(&EngineConfigXmlConverter::convertCapCriterionToAidl, this,
                                  std::placeholders::_1));
        capSpecificConfig.criterionTypes = convertWrappedCollectionToAidlUnchecked<
                xsd::CriterionTypesType, xsd::CriterionTypeType, AudioHalCapCriterionType>(
                getXsdcConfig()->getCriterion_types(), &xsd::CriterionTypesType::getCriterion_type,
                std::bind(&EngineConfigXmlConverter::convertCapCriterionTypeToAidl, this,
                          std::placeholders::_1));
        mAidlEngineConfig.capSpecificConfig = capSpecificConfig;
    }
}
}  // namespace aidl::android::hardware::audio::core::internal
