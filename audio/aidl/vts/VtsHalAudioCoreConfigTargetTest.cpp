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

#include "AudioHalBinderServiceUtil.h"
#include "TestUtils.h"

using namespace android;
using aidl::android::hardware::audio::core::IConfig;
using aidl::android::media::audio::common::AudioAttributes;
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
     * Verify defaultLiteralValue is empty for inclusive criterion.
     */
    void ValidateAudioHalCapCriterion(const AudioHalCapCriterion& criterion,
                                      const AudioHalCapCriterionType& criterionType) {
        if (criterionType.isInclusive) {
            EXPECT_TRUE(criterion.defaultLiteralValue.empty());
        }
    }

    /**
     * Verify values only contain alphanumeric characters.
     */
    void ValidateAudioHalCapCriterionType(const AudioHalCapCriterionType& criterionType) {
        auto isNotAlnum = [](const char& c) { return !isalnum(c); };
        for (const std::string& value : criterionType.values) {
            EXPECT_EQ(find_if(value.begin(), value.end(), isNotAlnum), value.end());
        }
    }

    /**
     * Verify each criterionType has a unique name.
     * Verify each criterion has a unique name.
     * Verify each criterion maps to a criterionType.
     * Verify each criterionType is used in a criterion.
     * Validate contained types.
     */
    void ValidateCapSpecificConfig(const AudioHalEngineConfig::CapSpecificConfig& capCfg) {
        EXPECT_FALSE(capCfg.criteria.empty());
        EXPECT_FALSE(capCfg.criterionTypes.empty());
        std::unordered_map<std::string, AudioHalCapCriterionType> criterionTypeMap;
        for (const AudioHalCapCriterionType& criterionType : capCfg.criterionTypes) {
            EXPECT_NO_FATAL_FAILURE(ValidateAudioHalCapCriterionType(criterionType));
            EXPECT_TRUE(criterionTypeMap.insert({criterionType.name, criterionType}).second);
        }
        std::unordered_set<std::string> criterionNameSet;
        for (const AudioHalCapCriterion& criterion : capCfg.criteria) {
            EXPECT_TRUE(criterionNameSet.insert(criterion.name).second);
            EXPECT_EQ(criterionTypeMap.count(criterion.criterionTypeName), 1UL);
            EXPECT_NO_FATAL_FAILURE(ValidateAudioHalCapCriterion(
                    criterion, criterionTypeMap.at(criterion.criterionTypeName)));
        }
        EXPECT_EQ(criterionTypeMap.size(), criterionNameSet.size());
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

  private:
    std::shared_ptr<IConfig> mConfig;
    std::unique_ptr<AudioHalEngineConfig> mEngineConfig;
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

INSTANTIATE_TEST_SUITE_P(AudioCoreConfigTest, AudioCoreConfig,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IConfig::descriptor)),
                         android::PrintInstanceNameToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AudioCoreConfig);
