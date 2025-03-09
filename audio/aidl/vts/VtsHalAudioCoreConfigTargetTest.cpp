/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define LOG_TAG "VtsHalAudioCore.Config"

#include <Utils.h>
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/audio/core/IConfig.h>
#include <aidl/android/media/audio/common/AudioFlag.h>
#include <aidl/android/media/audio/common/AudioProductStrategyType.h>
#include <android-base/logging.h>

#include "AudioHalBinderServiceUtil.h"
#include "TestUtils.h"

using namespace android;
using aidl::android::hardware::audio::common::isDefaultAudioFormat;
using aidl::android::hardware::audio::core::IConfig;
using aidl::android::hardware::audio::core::SurroundSoundConfig;
using aidl::android::media::audio::common::AudioAttributes;
using aidl::android::media::audio::common::AudioDeviceAddress;
using aidl::android::media::audio::common::AudioDeviceDescription;
using aidl::android::media::audio::common::AudioFlag;
using aidl::android::media::audio::common::AudioFormatDescription;
using aidl::android::media::audio::common::AudioFormatType;
using aidl::android::media::audio::common::AudioHalAttributesGroup;
using aidl::android::media::audio::common::AudioHalCapConfiguration;
using aidl::android::media::audio::common::AudioHalCapCriterionV2;
using aidl::android::media::audio::common::AudioHalCapDomain;
using aidl::android::media::audio::common::AudioHalCapParameter;
using aidl::android::media::audio::common::AudioHalCapRule;
using aidl::android::media::audio::common::AudioHalEngineConfig;
using aidl::android::media::audio::common::AudioHalProductStrategy;
using aidl::android::media::audio::common::AudioHalVolumeCurve;
using aidl::android::media::audio::common::AudioHalVolumeGroup;
using aidl::android::media::audio::common::AudioMode;
using aidl::android::media::audio::common::AudioPolicyForceUse;
using aidl::android::media::audio::common::AudioProductStrategyType;
using aidl::android::media::audio::common::AudioSource;
using aidl::android::media::audio::common::AudioStreamType;
using aidl::android::media::audio::common::AudioUsage;
using aidl::android::media::audio::common::PcmType;

class AudioCoreConfig : public testing::TestWithParam<std::string> {
  public:
    void SetUp() override { ASSERT_NO_FATAL_FAILURE(ConnectToService()); }
    void ConnectToService() {
        mConfig = IConfig::fromBinder(mBinderUtil.connectToService(GetParam()));
        ASSERT_NE(mConfig, nullptr);
    }

    void RestartService() {
        ASSERT_NE(mConfig, nullptr);
        mEngineConfig.reset();
        mSurroundSoundConfig.reset();
        mConfig = IConfig::fromBinder(mBinderUtil.restartService());
        ASSERT_NE(mConfig, nullptr);
    }

    void SetUpEngineConfig() {
        if (mEngineConfig == nullptr) {
            auto tempConfig = std::make_unique<AudioHalEngineConfig>();
            ASSERT_IS_OK(mConfig->getEngineConfig(tempConfig.get()));
            mEngineConfig = std::move(tempConfig);
        }
    }

    void SetUpSurroundSoundConfig() {
        if (mSurroundSoundConfig == nullptr) {
            auto tempConfig = std::make_unique<SurroundSoundConfig>();
            ASSERT_IS_OK(mConfig->getSurroundSoundConfig(tempConfig.get()));
            mSurroundSoundConfig = std::move(tempConfig);
        }
    }

    static bool IsProductStrategyTypeReservedForSystemUse(const AudioProductStrategyType& pst) {
        switch (pst) {
            case AudioProductStrategyType::SYS_RESERVED_NONE:
            case AudioProductStrategyType::SYS_RESERVED_REROUTING:
            case AudioProductStrategyType::SYS_RESERVED_CALL_ASSISTANT:
                return true;
            default:
                return false;
        }
    }

    static bool IsStreamTypeReservedForSystemUse(const AudioStreamType& streamType) {
        switch (streamType) {
            case AudioStreamType::SYS_RESERVED_DEFAULT:
            case AudioStreamType::SYS_RESERVED_REROUTING:
            case AudioStreamType::SYS_RESERVED_PATCH:
            case AudioStreamType::CALL_ASSISTANT:
                return true;
            default:
                return false;
        }
    }

    static bool IsAudioUsageValid(const AudioUsage& usage) {
        switch (usage) {
            case AudioUsage::INVALID:
            case AudioUsage::SYS_RESERVED_NOTIFICATION_COMMUNICATION_REQUEST:
            case AudioUsage::SYS_RESERVED_NOTIFICATION_COMMUNICATION_INSTANT:
            case AudioUsage::SYS_RESERVED_NOTIFICATION_COMMUNICATION_DELAYED:
                return false;
            default:
                return true;
        }
    }

    static bool IsAudioSourceValid(const AudioSource& source) {
        return (source != AudioSource::SYS_RESERVED_INVALID);
    }

    static const std::unordered_set<int>& GetSupportedAudioProductStrategyTypes() {
        static const std::unordered_set<int> supportedAudioProductStrategyTypes = []() {
            std::unordered_set<int> supportedStrategyTypes;
            for (const auto& audioProductStrategyType :
                 ndk::enum_range<AudioProductStrategyType>()) {
                if (!IsProductStrategyTypeReservedForSystemUse(audioProductStrategyType)) {
                    supportedStrategyTypes.insert(static_cast<int>(audioProductStrategyType));
                }
            }
            return supportedStrategyTypes;
        }();
        return supportedAudioProductStrategyTypes;
    }

    static int GetSupportedAudioFlagsMask() {
        static const int supportedAudioFlagsMask = []() {
            int mask = 0;
            for (const auto& audioFlag : ndk::enum_range<AudioFlag>()) {
                mask |= static_cast<int>(audioFlag);
            }
            return mask;
        }();
        return supportedAudioFlagsMask;
    }

    /**
     * Verify streamType is not INVALID if using default engine.
     * Verify that streamType is a valid AudioStreamType if the associated
     * volumeGroup minIndex/maxIndex is INDEX_DEFERRED_TO_AUDIO_SERVICE.
     */
    void ValidateAudioStreamType(const AudioStreamType& streamType,
                                 const AudioHalVolumeGroup& associatedVolumeGroup) {
        EXPECT_FALSE(IsStreamTypeReservedForSystemUse(streamType));
        if (!mEngineConfig->capSpecificConfig ||
            associatedVolumeGroup.minIndex ==
                    AudioHalVolumeGroup::INDEX_DEFERRED_TO_AUDIO_SERVICE) {
            EXPECT_NE(streamType, AudioStreamType::INVALID);
        }
    }

    /**
     * Verify contained enum types are valid.
     */
    void ValidateAudioAttributes(const AudioAttributes& attributes) {
        // No need to check contentType; there are no INVALID or SYS_RESERVED values
        EXPECT_TRUE(IsAudioUsageValid(attributes.usage));
        EXPECT_TRUE(IsAudioSourceValid(attributes.source));
        EXPECT_EQ(attributes.flags & ~GetSupportedAudioFlagsMask(), 0);
    }

    /**
     * Verify volumeGroupName corresponds to an AudioHalVolumeGroup.
     * Validate contained types.
     */
    void ValidateAudioHalAttributesGroup(
            const AudioHalAttributesGroup& attributesGroup,
            std::unordered_map<std::string, const AudioHalVolumeGroup&>& volumeGroupMap,
            std::unordered_set<std::string>& volumeGroupsUsedInStrategies) {
        bool isVolumeGroupNameValid = volumeGroupMap.count(attributesGroup.volumeGroupName);
        EXPECT_TRUE(isVolumeGroupNameValid);
        EXPECT_NO_FATAL_FAILURE(ValidateAudioStreamType(
                attributesGroup.streamType, volumeGroupMap.at(attributesGroup.volumeGroupName)));
        if (isVolumeGroupNameValid) {
            volumeGroupsUsedInStrategies.insert(attributesGroup.volumeGroupName);
        }
        for (const AudioAttributes& attr : attributesGroup.attributes) {
            EXPECT_NO_FATAL_FAILURE(ValidateAudioAttributes(attr));
        }
    }

    /**
     * Default engine: verify productStrategy.id is valid AudioProductStrategyType.
     * CAP engine: verify productStrategy.id is either valid AudioProductStrategyType
     * or is >= VENDOR_STRATEGY_ID_START.
     * Validate contained types.
     */
    void ValidateAudioHalProductStrategy(
            const AudioHalProductStrategy& strategy,
            std::unordered_map<std::string, const AudioHalVolumeGroup&>& volumeGroupMap,
            std::unordered_set<std::string>& volumeGroupsUsedInStrategies) {
        if (!mEngineConfig->capSpecificConfig ||
            (strategy.id < AudioHalProductStrategy::VENDOR_STRATEGY_ID_START)) {
            EXPECT_NE(GetSupportedAudioProductStrategyTypes().find(strategy.id),
                      GetSupportedAudioProductStrategyTypes().end());
        }
        for (const AudioHalAttributesGroup& attributesGroup : strategy.attributesGroups) {
            EXPECT_NO_FATAL_FAILURE(ValidateAudioHalAttributesGroup(attributesGroup, volumeGroupMap,
                                                                    volumeGroupsUsedInStrategies));
        }
    }

    /**
     * Verify curve point index is in [CurvePoint::MIN_INDEX, CurvePoint::MAX_INDEX].
     */
    void ValidateAudioHalVolumeCurve(const AudioHalVolumeCurve& volumeCurve) {
        for (const AudioHalVolumeCurve::CurvePoint& curvePoint : volumeCurve.curvePoints) {
            EXPECT_TRUE(curvePoint.index >= AudioHalVolumeCurve::CurvePoint::MIN_INDEX);
            EXPECT_TRUE(curvePoint.index <= AudioHalVolumeCurve::CurvePoint::MAX_INDEX);
        }
    }

    /**
     * Verify minIndex, maxIndex are non-negative.
     * Verify minIndex <= maxIndex.
     * Verify no two volume curves use the same device category.
     * Validate contained types.
     */
    void ValidateAudioHalVolumeGroup(const AudioHalVolumeGroup& volumeGroup) {
        /**
         * Legacy volume curves in audio_policy_configuration.xsd don't use
         * minIndex or maxIndex. Use of audio_policy_configuration.xml still
         * allows, and in some cases, relies on, AudioService to provide the min
         * and max indices for a volumeGroup. From the VTS perspective, there is
         * no way to differentiate between use of audio_policy_configuration.xml
         * or audio_policy_engine_configuration.xml, as either one can be used
         * for the default audio policy engine.
         */
        if (volumeGroup.minIndex != AudioHalVolumeGroup::INDEX_DEFERRED_TO_AUDIO_SERVICE ||
            volumeGroup.maxIndex != AudioHalVolumeGroup::INDEX_DEFERRED_TO_AUDIO_SERVICE) {
            EXPECT_TRUE(volumeGroup.minIndex >= 0);
            EXPECT_TRUE(volumeGroup.maxIndex >= 0);
        }
        EXPECT_TRUE(volumeGroup.minIndex <= volumeGroup.maxIndex);
        std::unordered_set<AudioHalVolumeCurve::DeviceCategory> deviceCategorySet;
        for (const AudioHalVolumeCurve& volumeCurve : volumeGroup.volumeCurves) {
            EXPECT_TRUE(deviceCategorySet.insert(volumeCurve.deviceCategory).second);
            EXPECT_NO_FATAL_FAILURE(ValidateAudioHalVolumeCurve(volumeCurve));
        }
    }

    /**
     * Verify criterion provides a non empty value list.
     * Verify logic rule provided is the expected one.
     */
    void ValidateAudioHalCapCriterion(const AudioHalCapCriterionV2& criterionV2) {
        switch (criterionV2.getTag()) {
            case AudioHalCapCriterionV2::availableInputDevices: {
                auto criterion = criterionV2.get<AudioHalCapCriterionV2::availableInputDevices>();
                EXPECT_FALSE(criterion.values.empty());
                EXPECT_EQ(criterion.logic, AudioHalCapCriterionV2::LogicalDisjunction::INCLUSIVE);
                break;
            }
            case AudioHalCapCriterionV2::availableOutputDevices: {
                auto criterion = criterionV2.get<AudioHalCapCriterionV2::availableOutputDevices>();
                EXPECT_FALSE(criterion.values.empty());
                EXPECT_EQ(criterion.logic, AudioHalCapCriterionV2::LogicalDisjunction::INCLUSIVE);
                break;
            }
            case AudioHalCapCriterionV2::availableInputDevicesAddresses: {
                auto criterion =
                        criterionV2.get<AudioHalCapCriterionV2::availableInputDevicesAddresses>();
                EXPECT_FALSE(criterion.values.empty());
                EXPECT_EQ(criterion.logic, AudioHalCapCriterionV2::LogicalDisjunction::INCLUSIVE);
                break;
            }
            case AudioHalCapCriterionV2::availableOutputDevicesAddresses: {
                auto criterion =
                        criterionV2.get<AudioHalCapCriterionV2::availableOutputDevicesAddresses>();
                EXPECT_FALSE(criterion.values.empty());
                EXPECT_EQ(criterion.logic, AudioHalCapCriterionV2::LogicalDisjunction::INCLUSIVE);
                break;
            }
            case AudioHalCapCriterionV2::telephonyMode: {
                auto criterion = criterionV2.get<AudioHalCapCriterionV2::telephonyMode>();
                EXPECT_FALSE(criterion.values.empty());
                EXPECT_EQ(criterion.logic, AudioHalCapCriterionV2::LogicalDisjunction::EXCLUSIVE);
                break;
            }
            case AudioHalCapCriterionV2::forceConfigForUse: {
                auto criterion = criterionV2.get<AudioHalCapCriterionV2::forceConfigForUse>();
                EXPECT_FALSE(criterion.values.empty());
                EXPECT_EQ(criterion.logic, AudioHalCapCriterionV2::LogicalDisjunction::EXCLUSIVE);
                break;
            }
            default:
                ADD_FAILURE() << "Invalid criterion tag " << toString(criterionV2.getTag());
        }
    }

    /**
     * Verify the rule involve the right matching logic according to the criterion logic.
     * @param matchingRule logic followed by the rule
     * @param logicalDisjunction logic exposed by the criterion
     */
    void ValidateAudioHalCapRuleMatchingRule(
            const AudioHalCapRule::MatchingRule matchingRule,
            const AudioHalCapCriterionV2::LogicalDisjunction logicalDisjunction) {
        if (logicalDisjunction == AudioHalCapCriterionV2::LogicalDisjunction::INCLUSIVE) {
            EXPECT_TRUE(matchingRule == AudioHalCapRule::MatchingRule::EXCLUDES ||
                        matchingRule == AudioHalCapRule::MatchingRule::INCLUDES);
        } else if (logicalDisjunction == AudioHalCapCriterionV2::LogicalDisjunction::EXCLUSIVE) {
            EXPECT_TRUE(matchingRule == AudioHalCapRule::MatchingRule::IS ||
                        matchingRule == AudioHalCapRule::MatchingRule::IS_NOT);
        } else {
            ADD_FAILURE() << "Invalid criterion Logical rule";
        }
    }

    /**
     * Verify that the value and the matching rule are supported by the given criterion
     */
    template <typename CriterionV2, typename Value>
    void validateAudioHalCapRule(CriterionV2 criterionV2, Value value,
                                 const AudioHalCapRule::MatchingRule matchingRule) {
        ValidateAudioHalCapRuleMatchingRule(matchingRule, criterionV2.logic);
        EXPECT_FALSE(criterionV2.values.empty());
        auto values = criterionV2.values;
        auto valueIt = find_if(values.begin(), values.end(),
                               [&](const auto& typedValue) { return typedValue == value; });
        EXPECT_NE(valueIt, values.end());
    }

    /**
     * Verify rule involves a supported criterion.
     * Verify rule involves supported logic keyword according to logic rule exposed by the
     * criterion.
     * Verify rule involves a value supported by the associated criterion.
     */
    void ValidateAudioHalConfigurationRule(
            const AudioHalCapRule& rule,
            const std::vector<std::optional<AudioHalCapCriterionV2>>& criteria) {
        const auto& compoundRule = rule.compoundRule;
        if (rule.nestedRules.empty() && rule.criterionRules.empty()) {
            EXPECT_EQ(compoundRule, AudioHalCapRule::CompoundRule::ALL);
        }
        EXPECT_TRUE(compoundRule == AudioHalCapRule::CompoundRule::ANY ||
                    compoundRule == AudioHalCapRule::CompoundRule::ALL);
        for (const auto& nestedRule : rule.nestedRules) {
            ValidateAudioHalConfigurationRule(nestedRule, criteria);
        }
        for (const auto& criterionRule : rule.criterionRules) {
            auto selectionCriterion = criterionRule.criterionAndValue;
            auto criterionValue = criterionRule.criterionAndValue;
            auto matchesWhen = criterionRule.matchingRule;
            auto criteriaIt = find_if(criteria.begin(), criteria.end(), [&](const auto& criterion) {
                return criterion.has_value() &&
                       criterion.value().getTag() == selectionCriterion.getTag();
            });
            EXPECT_NE(criteriaIt, criteria.end())
                    << " Invalid rule criterion " << toString(selectionCriterion.getTag());
            AudioHalCapCriterionV2 matchingCriterion = (*criteriaIt).value();
            switch (selectionCriterion.getTag()) {
                case AudioHalCapCriterionV2::availableInputDevices: {
                    const auto& values =
                            criterionValue.get<AudioHalCapCriterionV2::availableInputDevices>()
                                    .values;
                    ASSERT_FALSE(values.empty());
                    validateAudioHalCapRule(
                            matchingCriterion.get<AudioHalCapCriterionV2::availableInputDevices>(),
                            values[0], matchesWhen);
                    break;
                }
                case AudioHalCapCriterionV2::availableOutputDevices: {
                    const auto& values =
                            criterionValue.get<AudioHalCapCriterionV2::availableOutputDevices>()
                                    .values;
                    ASSERT_FALSE(values.empty());
                    validateAudioHalCapRule(
                            matchingCriterion.get<AudioHalCapCriterionV2::availableOutputDevices>(),
                            values[0], matchesWhen);
                    break;
                }
                case AudioHalCapCriterionV2::availableInputDevicesAddresses: {
                    const auto& values =
                            criterionValue
                                    .get<AudioHalCapCriterionV2::availableInputDevicesAddresses>()
                                    .values;
                    ASSERT_FALSE(values.empty());
                    validateAudioHalCapRule(
                            matchingCriterion
                                    .get<AudioHalCapCriterionV2::availableInputDevicesAddresses>(),
                            values[0], matchesWhen);
                    break;
                }
                case AudioHalCapCriterionV2::availableOutputDevicesAddresses: {
                    const auto& values =
                            criterionValue
                                    .get<AudioHalCapCriterionV2::availableOutputDevicesAddresses>()
                                    .values;
                    ASSERT_FALSE(values.empty());
                    validateAudioHalCapRule(
                            matchingCriterion
                                    .get<AudioHalCapCriterionV2::availableOutputDevicesAddresses>(),
                            values[0], matchesWhen);
                    break;
                }
                case AudioHalCapCriterionV2::telephonyMode: {
                    const auto& values =
                            criterionValue.get<AudioHalCapCriterionV2::telephonyMode>().values;
                    ASSERT_FALSE(values.empty());
                    validateAudioHalCapRule(
                            matchingCriterion.get<AudioHalCapCriterionV2::telephonyMode>(),
                            values[0], matchesWhen);
                    break;
                }
                case AudioHalCapCriterionV2::forceConfigForUse: {
                    const auto& values =
                            criterionValue.get<AudioHalCapCriterionV2::forceConfigForUse>().values;
                    ASSERT_FALSE(values.empty());
                    validateAudioHalCapRule(
                            matchingCriterion.get<AudioHalCapCriterionV2::forceConfigForUse>(),
                            values[0], matchesWhen);
                    break;
                }
                default:
                    break;
            }
        }
    }

    /**
     * Get the number of occurrence of a given parameter within a given vector of parameter.
     * It just take into account the parameter, not its associated value.
     * @param parameter to consider
     * @param domainParameters to check against
     * @return matching occurrence of the parameter within the provided vector.
     */
    size_t countsParameter(const AudioHalCapParameter& parameter,
                           const std::vector<AudioHalCapParameter>& domainParameters) {
        size_t count = 0;
        for (const auto& domainParameter : domainParameters) {
            if (domainParameter.getTag() != parameter.getTag()) {
                continue;
            }
            switch (domainParameter.getTag()) {
                case AudioHalCapParameter::selectedStrategyDevice: {
                    auto typedDomainParam =
                            domainParameter.get<AudioHalCapParameter::selectedStrategyDevice>();
                    auto typedParam = parameter.get<AudioHalCapParameter::selectedStrategyDevice>();
                    if (typedDomainParam.id == typedParam.id &&
                        typedDomainParam.device == typedParam.device) {
                        count += 1;
                    }
                    break;
                }
                case AudioHalCapParameter::strategyDeviceAddress: {
                    auto typedDomainParam =
                            domainParameter.get<AudioHalCapParameter::strategyDeviceAddress>();
                    auto typedParam = parameter.get<AudioHalCapParameter::strategyDeviceAddress>();
                    if (typedDomainParam.id == typedParam.id) {
                        count += 1;
                    }
                    break;
                }
                case AudioHalCapParameter::selectedInputSourceDevice: {
                    auto typedDomainParam =
                            domainParameter.get<AudioHalCapParameter::selectedInputSourceDevice>();
                    auto typedParam =
                            parameter.get<AudioHalCapParameter::selectedInputSourceDevice>();
                    if (typedDomainParam.inputSource == typedParam.inputSource &&
                        typedDomainParam.device == typedParam.device) {
                        count += 1;
                    }
                    break;
                }
                case AudioHalCapParameter::streamVolumeProfile: {
                    auto typedDomainParam =
                            domainParameter.get<AudioHalCapParameter::streamVolumeProfile>();
                    auto typedParam = parameter.get<AudioHalCapParameter::streamVolumeProfile>();
                    if (typedDomainParam.stream == typedParam.stream) {
                        count += 1;
                    }
                    break;
                }
                default:
                    break;
            }
        }
        return count;
    }

    /**
     * Verify each configuration has unique name within a domain
     * Verify no duplicate parameter within a domain.
     * Verify that each configuration has no duplicated parameter.
     * Verify that each configuration has an associated value for all parameter within a domain.
     */
    void ValidateAudioHalCapDomain(
            const AudioHalCapDomain& domain,
            const std::vector<std::optional<AudioHalCapCriterionV2>>& criteria) {
        std::unordered_set<std::string> configurationNames;
        for (const AudioHalCapConfiguration& configuration : domain.configurations) {
            EXPECT_TRUE(configurationNames.insert(configuration.name).second);
            ValidateAudioHalConfigurationRule(configuration.rule, criteria);
        }
        auto domainParameters = domain.configurations[0].parameterSettings;
        for (const auto& settingParameter : domainParameters) {
            EXPECT_EQ(1ul, countsParameter(settingParameter, domainParameters))
                    << "Duplicated parameter within domain " << domain.name << " configuration "
                    << domain.configurations[0].name << " for parameter "
                    << settingParameter.toString();
        }
        for (const auto& configuration : domain.configurations) {
            auto configurationParameters = configuration.parameterSettings;
            for (const auto& configurationParameter : configurationParameters) {
                EXPECT_EQ(1ul, countsParameter(configurationParameter, configurationParameters))
                        << "Duplicated parameter within domain " << domain.name << " configuration "
                        << configuration.name << " for parameter "
                        << configurationParameter.toString();
            }
            EXPECT_EQ(domainParameters.size(), configurationParameters.size());
            for (const auto& settingParameter : configuration.parameterSettings) {
                EXPECT_EQ(1ul, countsParameter(settingParameter, domainParameters))
                        << "Confiugration " << configuration.name << " within domain "
                        << domain.name << " exposes invalid parameter "
                        << settingParameter.toString();
                ;
            }
        }
    }

    /**
     * Verify each domain has a unique name.
     * Verify that a given parameter does not appear in more than one domain.
     */
    void ValidateAudioHalCapDomains(
            const std::vector<std::optional<AudioHalCapDomain>>& domains,
            const std::vector<std::optional<AudioHalCapCriterionV2>>& criteria) {
        std::unordered_map<std::string, AudioHalCapDomain> domainMap;
        std::vector<AudioHalCapParameter> allDomainParameters;
        for (const auto& domain : domains) {
            EXPECT_TRUE(domain.has_value());
            EXPECT_FALSE(domain.value().configurations.empty());
            auto domainParameters = domain.value().configurations[0].parameterSettings;
            for (const auto& domainParameter : domainParameters) {
                EXPECT_EQ(0ul, countsParameter(domainParameter, allDomainParameters))
                        << "Duplicated parameter in domain " << domain.value().name
                        << " for parameter " << domainParameter.toString();
                allDomainParameters.push_back(domainParameter);
            }
            EXPECT_NO_FATAL_FAILURE(ValidateAudioHalCapDomain(domain.value(), criteria));
            EXPECT_TRUE(domainMap.insert({domain.value().name, domain.value()}).second);
        }
    }

    /**
     * Verify unique criterion is provided for a given Tag, except for ForceUse
     * Verify unique forceUse criterion are provided for usage
     * Verify each criterion is validating.
     * Verify domains.
     */
    void ValidateCapSpecificConfig(const AudioHalEngineConfig::CapSpecificConfig& capCfg) {
        EXPECT_TRUE(capCfg.criteriaV2.has_value());
        std::unordered_set<AudioHalCapCriterionV2::Tag> criterionTagSet;
        std::unordered_set<AudioPolicyForceUse::Tag> forceUseCriterionUseSet;
        for (const auto& criterion : capCfg.criteriaV2.value()) {
            EXPECT_TRUE(criterion.has_value());
            if (criterion.value().getTag() != AudioHalCapCriterionV2::forceConfigForUse) {
                EXPECT_TRUE(criterionTagSet.insert(criterion.value().getTag()).second);
            } else {
                auto forceUseCriterion =
                        criterion.value().get<AudioHalCapCriterionV2::forceConfigForUse>();
                ASSERT_FALSE(forceUseCriterion.values.empty());
                EXPECT_TRUE(forceUseCriterionUseSet.insert(forceUseCriterion.values[0].getTag())
                                    .second);
            }
            EXPECT_NO_FATAL_FAILURE(ValidateAudioHalCapCriterion(criterion.value()));
        }
        ValidateAudioHalCapDomains(capCfg.domains.value(), capCfg.criteriaV2.value());
    }

    /**
     * Verify VolumeGroups are non-empty.
     * Verify defaultProductStrategyId matches one of the provided productStrategies.
     * Otherwise, must be left uninitialized.
     * Verify each volumeGroup has a unique name.
     * Verify each productStrategy has a unique id.
     * Verify each volumeGroup is used in a product strategy.
     * CAP engine: verify productStrategies are non-empty.
     * Validate contained types.
     */
    void ValidateAudioHalEngineConfig() {
        EXPECT_NE(mEngineConfig->volumeGroups.size(), 0UL);
        std::unordered_map<std::string, const AudioHalVolumeGroup&> volumeGroupMap;
        for (const AudioHalVolumeGroup& volumeGroup : mEngineConfig->volumeGroups) {
            EXPECT_TRUE(volumeGroupMap.insert({volumeGroup.name, volumeGroup}).second);
            EXPECT_NO_FATAL_FAILURE(ValidateAudioHalVolumeGroup(volumeGroup));
        }
        if (!mEngineConfig->productStrategies.empty()) {
            std::unordered_set<int> productStrategyIdSet;
            std::unordered_set<std::string> volumeGroupsUsedInStrategies;
            for (const AudioHalProductStrategy& strategy : mEngineConfig->productStrategies) {
                EXPECT_TRUE(productStrategyIdSet.insert(strategy.id).second);
                EXPECT_NO_FATAL_FAILURE(ValidateAudioHalProductStrategy(
                        strategy, volumeGroupMap, volumeGroupsUsedInStrategies));
            }
            EXPECT_TRUE(productStrategyIdSet.count(mEngineConfig->defaultProductStrategyId))
                    << "defaultProductStrategyId doesn't match any of the provided "
                       "productStrategies";
            EXPECT_EQ(volumeGroupMap.size(), volumeGroupsUsedInStrategies.size());
        } else {
            EXPECT_EQ(mEngineConfig->defaultProductStrategyId,
                      static_cast<int>(AudioProductStrategyType::SYS_RESERVED_NONE))
                    << "defaultProductStrategyId defined, but no productStrategies were provided";
        }
        if (mEngineConfig->capSpecificConfig) {
            EXPECT_NO_FATAL_FAILURE(
                    ValidateCapSpecificConfig(mEngineConfig->capSpecificConfig.value()));
            EXPECT_FALSE(mEngineConfig->productStrategies.empty());
        }
    }

    void ValidateAudioFormatDescription(const AudioFormatDescription& format) {
        EXPECT_NE(AudioFormatType::SYS_RESERVED_INVALID, format.type);
        if (format.type == AudioFormatType::PCM) {
            EXPECT_NE(PcmType::DEFAULT, format.pcm);
            EXPECT_TRUE(format.encoding.empty()) << format.encoding;
        } else {
            EXPECT_FALSE(format.encoding.empty());
        }
    }

    /**
     * Verify that the surround sound configuration is not empty.
     * Verify each of the formatFamilies has a non-empty primaryFormat.
     * Verify that each format only appears once.
     */
    void ValidateSurroundSoundConfig() {
        EXPECT_FALSE(mSurroundSoundConfig->formatFamilies.empty());
        std::set<AudioFormatDescription> formatSet;
        for (const SurroundSoundConfig::SurroundFormatFamily& family :
             mSurroundSoundConfig->formatFamilies) {
            EXPECT_NO_FATAL_FAILURE(ValidateAudioFormatDescription(family.primaryFormat));
            EXPECT_FALSE(isDefaultAudioFormat(family.primaryFormat));
            EXPECT_TRUE(formatSet.insert(family.primaryFormat).second);
            for (const AudioFormatDescription& subformat : family.subFormats) {
                EXPECT_NO_FATAL_FAILURE(ValidateAudioFormatDescription(subformat));
                EXPECT_FALSE(isDefaultAudioFormat(subformat));
                EXPECT_TRUE(formatSet.insert(subformat).second);
            }
        }
    }

  private:
    std::shared_ptr<IConfig> mConfig;
    std::unique_ptr<AudioHalEngineConfig> mEngineConfig;
    std::unique_ptr<SurroundSoundConfig> mSurroundSoundConfig;
    AudioHalBinderServiceUtil mBinderUtil;
};

TEST_P(AudioCoreConfig, Published) {
    // SetUp must complete with no failures.
}

TEST_P(AudioCoreConfig, CanBeRestarted) {
    ASSERT_NO_FATAL_FAILURE(RestartService());
}

TEST_P(AudioCoreConfig, GetEngineConfigIsValid) {
    ASSERT_NO_FATAL_FAILURE(SetUpEngineConfig());
    EXPECT_NO_FATAL_FAILURE(ValidateAudioHalEngineConfig());
}

TEST_P(AudioCoreConfig, GetSurroundSoundConfigIsValid) {
    ASSERT_NO_FATAL_FAILURE(SetUpSurroundSoundConfig());
    EXPECT_NO_FATAL_FAILURE(ValidateSurroundSoundConfig());
}

INSTANTIATE_TEST_SUITE_P(AudioCoreConfigTest, AudioCoreConfig,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IConfig::descriptor)),
                         android::PrintInstanceNameToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AudioCoreConfig);
