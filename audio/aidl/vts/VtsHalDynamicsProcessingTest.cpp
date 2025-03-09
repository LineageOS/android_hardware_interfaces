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
#include <unordered_set>

#define LOG_TAG "VtsHalDynamicsProcessingTest"
#include <android-base/logging.h>
#include <audio_utils/power.h>
#include <audio_utils/primitives.h>

#include <Utils.h>

#include "EffectHelper.h"
#include "EffectRangeSpecific.h"

using namespace android;
using namespace aidl::android::hardware::audio::effect::DynamicsProcessingRanges;

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::DynamicsProcessing;
using aidl::android::hardware::audio::effect::getEffectTypeUuidDynamicsProcessing;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;
using android::hardware::audio::common::testing::detail::TestExecutionTracer;

/**
 * Here we focus on specific parameter checking, general IEffect interfaces testing performed in
 * VtsAudioEffectTargetTest.
 */
class DynamicsProcessingTestHelper : public EffectHelper {
  public:
    DynamicsProcessingTestHelper(std::pair<std::shared_ptr<IFactory>, Descriptor> pair,
                                 int32_t channelLayOut = AudioChannelLayout::LAYOUT_STEREO)
        : mChannelLayout(channelLayOut),
          mChannelCount(::aidl::android::hardware::audio::common::getChannelCount(
                  AudioChannelLayout::make<AudioChannelLayout::layoutMask>(mChannelLayout))) {
        std::tie(mFactory, mDescriptor) = pair;
    }

    // setup
    void SetUpDynamicsProcessingEffect() {
        ASSERT_NE(nullptr, mFactory);
        ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
        Parameter::Specific specific = getDefaultParamSpecific();
        Parameter::Common common = createParamCommon(
                0 /* session */, 1 /* ioHandle */, kSamplingFrequency /* iSampleRate */,
                kSamplingFrequency /* oSampleRate */, kFrameCount /* iFrameCount */,
                kFrameCount /* oFrameCount */,
                AudioChannelLayout::make<AudioChannelLayout::layoutMask>(mChannelLayout),
                AudioChannelLayout::make<AudioChannelLayout::layoutMask>(mChannelLayout));
        ASSERT_NO_FATAL_FAILURE(open(mEffect, common, specific, &mOpenEffectReturn, EX_NONE));
        ASSERT_NE(nullptr, mEffect);
        mEngineConfigApplied = mEngineConfigPreset;
    }

    Parameter::Specific getDefaultParamSpecific() {
        DynamicsProcessing dp = DynamicsProcessing::make<DynamicsProcessing::engineArchitecture>(
                mEngineConfigPreset);
        Parameter::Specific specific =
                Parameter::Specific::make<Parameter::Specific::dynamicsProcessing>(dp);
        return specific;
    }

    // teardown
    void TearDownDynamicsProcessingEffect() {
        ASSERT_NO_FATAL_FAILURE(close(mEffect));
        ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
    }

    // utils functions for parameter checking
    bool isParamEqual(const DynamicsProcessing::Tag& tag, const DynamicsProcessing& dpRef,
                      const DynamicsProcessing& dpTest);
    bool isEngineConfigEqual(const DynamicsProcessing::EngineArchitecture& refCfg,
                             const DynamicsProcessing::EngineArchitecture& testCfg);

    template <typename T>
    std::vector<T> filterEnabledVector(const std::vector<T>& vec);

    template <typename T>
    bool isAidlVectorEqualAfterFilter(const std::vector<T>& source, const std::vector<T>& target);

    template <typename T>
    bool isAidlVectorEqual(const std::vector<T>& source, const std::vector<T>& target);

    template <typename T>
    bool isChannelConfigValid(const std::vector<T>& cfgs) {
        auto& channelCount = mChannelCount;
        return std::all_of(cfgs.cbegin(), cfgs.cend(), [channelCount](const T& cfg) {
            return (cfg.channel >= 0 && cfg.channel < channelCount);
        });
    }

    template <typename T>
    bool isBandConfigValid(const std::vector<T>& cfgs, int bandCount);

    bool isParamValid(const DynamicsProcessing::Tag& tag, const DynamicsProcessing& dp);

    // get set params and validate
    void SetAndGetDynamicsProcessingParameters();

    bool isAllParamsValid();

    // enqueue test parameters
    void addEngineConfig(const DynamicsProcessing::EngineArchitecture& cfg);
    void addPreEqChannelConfig(const std::vector<DynamicsProcessing::ChannelConfig>& cfg);
    void addPostEqChannelConfig(const std::vector<DynamicsProcessing::ChannelConfig>& cfg);
    void addMbcChannelConfig(const std::vector<DynamicsProcessing::ChannelConfig>& cfg);
    void addPreEqBandConfigs(const std::vector<DynamicsProcessing::EqBandConfig>& cfgs);
    void addPostEqBandConfigs(const std::vector<DynamicsProcessing::EqBandConfig>& cfgs);
    void addMbcBandConfigs(const std::vector<DynamicsProcessing::MbcBandConfig>& cfgs);
    void addLimiterConfig(const std::vector<DynamicsProcessing::LimiterConfig>& cfg);
    void addInputGain(const std::vector<DynamicsProcessing::InputGain>& inputGain);

    static constexpr float kPreferredProcessingDurationMs = 10.0f;
    static constexpr int kBandCount = 5;
    static constexpr int kSamplingFrequency = 44100;
    static constexpr int kFrameCount = 2048;
    std::shared_ptr<IFactory> mFactory;
    std::shared_ptr<IEffect> mEffect;
    Descriptor mDescriptor;
    IEffect::OpenEffectReturn mOpenEffectReturn;
    DynamicsProcessing::EngineArchitecture mEngineConfigApplied;
    DynamicsProcessing::EngineArchitecture mEngineConfigPreset{
            .resolutionPreference =
                    DynamicsProcessing::ResolutionPreference::FAVOR_FREQUENCY_RESOLUTION,
            .preferredProcessingDurationMs = kPreferredProcessingDurationMs,
            .preEqStage = {.inUse = true, .bandCount = kBandCount},
            .postEqStage = {.inUse = true, .bandCount = kBandCount},
            .mbcStage = {.inUse = true, .bandCount = kBandCount},
            .limiterInUse = true,
    };

    std::unordered_set<int /* channelId */> mPreEqChannelEnable;
    std::unordered_set<int /* channelId */> mPostEqChannelEnable;
    std::unordered_set<int /* channelId */> mMbcChannelEnable;
    std::unordered_set<int /* channelId */> mLimiterChannelEnable;
    static const std::set<std::vector<DynamicsProcessing::ChannelConfig>> kChannelConfigTestSet;
    static const std::set<DynamicsProcessing::StageEnablement> kStageEnablementTestSet;
    static const std::set<std::vector<DynamicsProcessing::InputGain>> kInputGainTestSet;

  private:
    const int32_t mChannelLayout;
    std::vector<std::pair<DynamicsProcessing::Tag, DynamicsProcessing>> mTags;

  protected:
    const int mChannelCount;
    void CleanUp() {
        mTags.clear();
        mPreEqChannelEnable.clear();
        mPostEqChannelEnable.clear();
        mMbcChannelEnable.clear();
        mLimiterChannelEnable.clear();
    }
};

// test value set for DynamicsProcessing::StageEnablement
const std::set<DynamicsProcessing::StageEnablement>
        DynamicsProcessingTestHelper::kStageEnablementTestSet = {
                {.inUse = true, .bandCount = DynamicsProcessingTestHelper::kBandCount},
                {.inUse = true, .bandCount = 0},
                {.inUse = true, .bandCount = -1},
                {.inUse = false, .bandCount = 0},
                {.inUse = false, .bandCount = -1},
                {.inUse = false, .bandCount = DynamicsProcessingTestHelper::kBandCount}};

// test value set for DynamicsProcessing::ChannelConfig
const std::set<std::vector<DynamicsProcessing::ChannelConfig>>
        DynamicsProcessingTestHelper::kChannelConfigTestSet = {
                {{.channel = -1, .enable = false},
                 {.channel = 0, .enable = true},
                 {.channel = 1, .enable = false},
                 {.channel = 2, .enable = true}},
                {{.channel = -1, .enable = false}, {.channel = 2, .enable = true}},
                {{.channel = 0, .enable = true}, {.channel = 1, .enable = true}}};

// test value set for DynamicsProcessing::InputGain
const std::set<std::vector<DynamicsProcessing::InputGain>>
        DynamicsProcessingTestHelper::kInputGainTestSet = {
                {{.channel = 0, .gainDb = 10.f},
                 {.channel = 1, .gainDb = 0.f},
                 {.channel = 2, .gainDb = -10.f}},
                {{.channel = -1, .gainDb = -10.f}, {.channel = -2, .gainDb = 10.f}},
                {{.channel = -1, .gainDb = 10.f}, {.channel = 0, .gainDb = -10.f}},
                {{.channel = 0, .gainDb = 10.f}, {.channel = 1, .gainDb = -10.f}}};

template <typename T>
bool DynamicsProcessingTestHelper::isBandConfigValid(const std::vector<T>& cfgs, int bandCount) {
    std::unordered_set<int> freqs;
    for (auto cfg : cfgs) {
        if (cfg.channel < 0 || cfg.channel >= mChannelCount) return false;
        if (cfg.band < 0 || cfg.band >= bandCount) return false;
        // duplicated band index
        if (freqs.find(cfg.band) != freqs.end()) return false;
        freqs.insert(cfg.band);
    }
    return true;
}

bool DynamicsProcessingTestHelper::isParamValid(const DynamicsProcessing::Tag& tag,
                                                const DynamicsProcessing& dp) {
    switch (tag) {
        case DynamicsProcessing::preEq: {
            return isChannelConfigValid(dp.get<DynamicsProcessing::preEq>());
        }
        case DynamicsProcessing::postEq: {
            return isChannelConfigValid(dp.get<DynamicsProcessing::postEq>());
        }
        case DynamicsProcessing::mbc: {
            return isChannelConfigValid(dp.get<DynamicsProcessing::mbc>());
        }
        case DynamicsProcessing::preEqBand: {
            return isBandConfigValid(dp.get<DynamicsProcessing::preEqBand>(),
                                     mEngineConfigApplied.preEqStage.bandCount);
        }
        case DynamicsProcessing::postEqBand: {
            return isBandConfigValid(dp.get<DynamicsProcessing::postEqBand>(),
                                     mEngineConfigApplied.postEqStage.bandCount);
        }
        case DynamicsProcessing::mbcBand: {
            return isBandConfigValid(dp.get<DynamicsProcessing::mbcBand>(),
                                     mEngineConfigApplied.mbcStage.bandCount);
        }
        case DynamicsProcessing::limiter: {
            return isChannelConfigValid(dp.get<DynamicsProcessing::limiter>());
        }
        case DynamicsProcessing::inputGain: {
            return isChannelConfigValid(dp.get<DynamicsProcessing::inputGain>());
        }
        default: {
            return true;
        }
    }
    return true;
}

bool DynamicsProcessingTestHelper::isParamEqual(const DynamicsProcessing::Tag& tag,
                                                const DynamicsProcessing& dpRef,
                                                const DynamicsProcessing& dpTest) {
    switch (tag) {
        case DynamicsProcessing::engineArchitecture: {
            return isEngineConfigEqual(dpRef.get<DynamicsProcessing::engineArchitecture>(),
                                       dpTest.get<DynamicsProcessing::engineArchitecture>());
        }
        case DynamicsProcessing::preEq: {
            const auto& source = dpRef.get<DynamicsProcessing::preEq>();
            const auto& target = dpTest.get<DynamicsProcessing::preEq>();
            return isAidlVectorEqualAfterFilter<DynamicsProcessing::ChannelConfig>(source, target);
        }
        case DynamicsProcessing::postEq: {
            return isAidlVectorEqualAfterFilter<DynamicsProcessing::ChannelConfig>(
                    dpRef.get<DynamicsProcessing::postEq>(),
                    dpTest.get<DynamicsProcessing::postEq>());
        }
        case DynamicsProcessing::mbc: {
            return isAidlVectorEqualAfterFilter<DynamicsProcessing::ChannelConfig>(
                    dpRef.get<DynamicsProcessing::mbc>(), dpTest.get<DynamicsProcessing::mbc>());
        }
        case DynamicsProcessing::preEqBand: {
            return isAidlVectorEqualAfterFilter<DynamicsProcessing::EqBandConfig>(
                    dpRef.get<DynamicsProcessing::preEqBand>(),
                    dpTest.get<DynamicsProcessing::preEqBand>());
        }
        case DynamicsProcessing::postEqBand: {
            return isAidlVectorEqualAfterFilter<DynamicsProcessing::EqBandConfig>(
                    dpRef.get<DynamicsProcessing::postEqBand>(),
                    dpTest.get<DynamicsProcessing::postEqBand>());
        }
        case DynamicsProcessing::mbcBand: {
            return isAidlVectorEqualAfterFilter<DynamicsProcessing::MbcBandConfig>(
                    dpRef.get<DynamicsProcessing::mbcBand>(),
                    dpTest.get<DynamicsProcessing::mbcBand>());
        }
        case DynamicsProcessing::limiter: {
            return isAidlVectorEqualAfterFilter<DynamicsProcessing::LimiterConfig>(
                    dpRef.get<DynamicsProcessing::limiter>(),
                    dpTest.get<DynamicsProcessing::limiter>());
        }
        case DynamicsProcessing::inputGain: {
            return isAidlVectorEqual<DynamicsProcessing::InputGain>(
                    dpRef.get<DynamicsProcessing::inputGain>(),
                    dpTest.get<DynamicsProcessing::inputGain>());
        }
        case DynamicsProcessing::vendor: {
            return false;
        }
    }
}

bool DynamicsProcessingTestHelper::isEngineConfigEqual(
        const DynamicsProcessing::EngineArchitecture& ref,
        const DynamicsProcessing::EngineArchitecture& test) {
    return ref == test;
}

template <typename T>
std::vector<T> DynamicsProcessingTestHelper::filterEnabledVector(const std::vector<T>& vec) {
    std::vector<T> ret;
    std::copy_if(vec.begin(), vec.end(), std::back_inserter(ret),
                 [](const auto& v) { return v.enable; });
    return ret;
}

template <typename T>
bool DynamicsProcessingTestHelper::isAidlVectorEqual(const std::vector<T>& source,
                                                     const std::vector<T>& target) {
    if (source.size() != target.size()) return false;

    auto tempS = source;
    auto tempT = target;
    std::sort(tempS.begin(), tempS.end());
    std::sort(tempT.begin(), tempT.end());
    return tempS == tempT;
}

template <typename T>
bool DynamicsProcessingTestHelper::isAidlVectorEqualAfterFilter(const std::vector<T>& source,
                                                                const std::vector<T>& target) {
    return isAidlVectorEqual<T>(filterEnabledVector<T>(source), filterEnabledVector<T>(target));
}

void DynamicsProcessingTestHelper::SetAndGetDynamicsProcessingParameters() {
    for (const auto& [tag, dp] : mTags) {
        // validate parameter
        Descriptor desc;
        ASSERT_STATUS(EX_NONE, mEffect->getDescriptor(&desc));
        bool valid = isParamInRange(dp, desc.capability.range.get<Range::dynamicsProcessing>());
        if (valid) valid = isParamValid(tag, dp);
        const binder_exception_t expected = valid ? EX_NONE : EX_ILLEGAL_ARGUMENT;

        // set parameter
        Parameter expectParam;
        Parameter::Specific specific;
        specific.set<Parameter::Specific::dynamicsProcessing>(dp);
        expectParam.set<Parameter::specific>(specific);
        ASSERT_STATUS(expected, mEffect->setParameter(expectParam))
                << "\n"
                << expectParam.toString() << "\n"
                << desc.toString();

        // only get if parameter in range and set success
        if (expected == EX_NONE) {
            Parameter getParam;
            Parameter::Id id;
            DynamicsProcessing::Id dpId;
            dpId.set<DynamicsProcessing::Id::commonTag>(tag);
            id.set<Parameter::Id::dynamicsProcessingTag>(dpId);
            // if set success, then get should match
            EXPECT_STATUS(expected, mEffect->getParameter(id, &getParam));
            Parameter::Specific specificTest = getParam.get<Parameter::specific>();
            const auto& target = specificTest.get<Parameter::Specific::dynamicsProcessing>();
            EXPECT_TRUE(isParamEqual(tag, dp, target)) << dp.toString() << "\n"
                                                       << target.toString();
            // update mEngineConfigApplied after setting successfully
            if (tag == DynamicsProcessing::engineArchitecture) {
                mEngineConfigApplied = target.get<DynamicsProcessing::engineArchitecture>();
            }
        }
    }
}

bool DynamicsProcessingTestHelper::isAllParamsValid() {
    if (mTags.empty()) {
        return false;
    }
    for (const auto& [tag, dp] : mTags) {
        // validate parameter
        if (!isParamInRange(dp, mDescriptor.capability.range.get<Range::dynamicsProcessing>())) {
            return false;
        }
        if (!isParamValid(tag, dp)) {
            return false;
        }
    }
    return true;
}

void DynamicsProcessingTestHelper::addEngineConfig(
        const DynamicsProcessing::EngineArchitecture& cfg) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::engineArchitecture>(cfg);
    mTags.push_back({DynamicsProcessing::engineArchitecture, dp});
}

void DynamicsProcessingTestHelper::addPreEqChannelConfig(
        const std::vector<DynamicsProcessing::ChannelConfig>& cfgs) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::preEq>(cfgs);
    mTags.push_back({DynamicsProcessing::preEq, dp});
    for (auto& cfg : cfgs) {
        if (cfg.enable) mPreEqChannelEnable.insert(cfg.channel);
    }
}

void DynamicsProcessingTestHelper::addPostEqChannelConfig(
        const std::vector<DynamicsProcessing::ChannelConfig>& cfgs) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::postEq>(cfgs);
    mTags.push_back({DynamicsProcessing::postEq, dp});
    for (auto& cfg : cfgs) {
        if (cfg.enable) mPostEqChannelEnable.insert(cfg.channel);
    }
}

void DynamicsProcessingTestHelper::addMbcChannelConfig(
        const std::vector<DynamicsProcessing::ChannelConfig>& cfgs) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::mbc>(cfgs);
    mTags.push_back({DynamicsProcessing::mbc, dp});
    for (auto& cfg : cfgs) {
        if (cfg.enable) mMbcChannelEnable.insert(cfg.channel);
    }
}

void DynamicsProcessingTestHelper::addPreEqBandConfigs(
        const std::vector<DynamicsProcessing::EqBandConfig>& cfgs) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::preEqBand>(cfgs);
    mTags.push_back({DynamicsProcessing::preEqBand, dp});
}

void DynamicsProcessingTestHelper::addPostEqBandConfigs(
        const std::vector<DynamicsProcessing::EqBandConfig>& cfgs) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::postEqBand>(cfgs);
    mTags.push_back({DynamicsProcessing::postEqBand, dp});
}

void DynamicsProcessingTestHelper::addMbcBandConfigs(
        const std::vector<DynamicsProcessing::MbcBandConfig>& cfgs) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::mbcBand>(cfgs);
    mTags.push_back({DynamicsProcessing::mbcBand, dp});
}

void DynamicsProcessingTestHelper::addLimiterConfig(
        const std::vector<DynamicsProcessing::LimiterConfig>& cfgs) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::limiter>(cfgs);
    mTags.push_back({DynamicsProcessing::limiter, dp});
    for (auto& cfg : cfgs) {
        if (cfg.enable) mLimiterChannelEnable.insert(cfg.channel);
    }
}

void DynamicsProcessingTestHelper::addInputGain(
        const std::vector<DynamicsProcessing::InputGain>& inputGains) {
    DynamicsProcessing dp;
    dp.set<DynamicsProcessing::inputGain>(inputGains);
    mTags.push_back({DynamicsProcessing::inputGain, dp});
}

void fillLimiterConfig(std::vector<DynamicsProcessing::LimiterConfig>& limiterConfigList,
                       int channelIndex, bool enable, int linkGroup, float attackTime,
                       float releaseTime, float ratio, float threshold, float postGain) {
    DynamicsProcessing::LimiterConfig cfg;
    cfg.channel = channelIndex;
    cfg.enable = enable;
    cfg.linkGroup = linkGroup;
    cfg.attackTimeMs = attackTime;
    cfg.releaseTimeMs = releaseTime;
    cfg.ratio = ratio;
    cfg.thresholdDb = threshold;
    cfg.postGainDb = postGain;
    limiterConfigList.push_back(cfg);
}

/**
 * Test DynamicsProcessing Engine Configuration
 */
enum EngineArchitectureTestParamName {
    ENGINE_TEST_INSTANCE_NAME,
    ENGINE_TEST_RESOLUTION_PREFERENCE,
    ENGINE_TEST_PREFERRED_DURATION,
    ENGINE_TEST_STAGE_ENABLEMENT
};
using EngineArchitectureTestParams = std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>,
                                                DynamicsProcessing::ResolutionPreference, float,
                                                DynamicsProcessing::StageEnablement>;

void fillEngineArchConfig(DynamicsProcessing::EngineArchitecture& cfg,
                          const EngineArchitectureTestParams& params) {
    cfg.resolutionPreference = std::get<ENGINE_TEST_RESOLUTION_PREFERENCE>(params);
    cfg.preferredProcessingDurationMs = std::get<ENGINE_TEST_PREFERRED_DURATION>(params);
    cfg.preEqStage = cfg.postEqStage = cfg.mbcStage =
            std::get<ENGINE_TEST_STAGE_ENABLEMENT>(params);
    cfg.limiterInUse = true;
}

class DynamicsProcessingTestEngineArchitecture
    : public ::testing::TestWithParam<EngineArchitectureTestParams>,
      public DynamicsProcessingTestHelper {
  public:
    DynamicsProcessingTestEngineArchitecture()
        : DynamicsProcessingTestHelper(std::get<ENGINE_TEST_INSTANCE_NAME>(GetParam())) {
        fillEngineArchConfig(mCfg, GetParam());
    };

    void SetUp() override { SetUpDynamicsProcessingEffect(); }

    void TearDown() override { TearDownDynamicsProcessingEffect(); }

    DynamicsProcessing::EngineArchitecture mCfg;
};

TEST_P(DynamicsProcessingTestEngineArchitecture, SetAndGetEngineArch) {
    EXPECT_NO_FATAL_FAILURE(addEngineConfig(mCfg));
    ASSERT_NO_FATAL_FAILURE(SetAndGetDynamicsProcessingParameters());
}

INSTANTIATE_TEST_SUITE_P(
        DynamicsProcessingTest, DynamicsProcessingTestEngineArchitecture,
        ::testing::Combine(
                testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                        IFactory::descriptor, getEffectTypeUuidDynamicsProcessing())),
                testing::Values(
                        DynamicsProcessing::ResolutionPreference::FAVOR_TIME_RESOLUTION,
                        DynamicsProcessing::ResolutionPreference::FAVOR_FREQUENCY_RESOLUTION,
                        static_cast<DynamicsProcessing::ResolutionPreference>(-1)),  // variant
                testing::Values(-10.f, 0.f, 10.f),  // processing duration
                testing::ValuesIn(
                        DynamicsProcessingTestHelper::kStageEnablementTestSet)  // preEQ/postEQ/mbc
                ),
        [](const auto& info) {
            auto descriptor = std::get<ENGINE_TEST_INSTANCE_NAME>(info.param).second;
            DynamicsProcessing::EngineArchitecture cfg;
            fillEngineArchConfig(cfg, info.param);
            std::string name = getPrefix(descriptor) + "_Cfg_" + cfg.toString();
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DynamicsProcessingTestEngineArchitecture);

/**
 * Test DynamicsProcessing Input Gain
 */
enum InputGainTestParamName {
    INPUT_GAIN_INSTANCE_NAME,
    INPUT_GAIN_PARAM,
};
class DynamicsProcessingTestInputGain
    : public ::testing::TestWithParam<std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>,
                                                 std::vector<DynamicsProcessing::InputGain>>>,
      public DynamicsProcessingTestHelper {
  public:
    DynamicsProcessingTestInputGain()
        : DynamicsProcessingTestHelper(std::get<INPUT_GAIN_INSTANCE_NAME>(GetParam())),
          mInputGain(std::get<INPUT_GAIN_PARAM>(GetParam())) {};

    void SetUp() override { SetUpDynamicsProcessingEffect(); }

    void TearDown() override { TearDownDynamicsProcessingEffect(); }

    const std::vector<DynamicsProcessing::InputGain> mInputGain;
};

TEST_P(DynamicsProcessingTestInputGain, SetAndGetInputGain) {
    EXPECT_NO_FATAL_FAILURE(addInputGain(mInputGain));
    ASSERT_NO_FATAL_FAILURE(SetAndGetDynamicsProcessingParameters());
}

INSTANTIATE_TEST_SUITE_P(
        DynamicsProcessingTest, DynamicsProcessingTestInputGain,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidDynamicsProcessing())),
                           testing::ValuesIn(DynamicsProcessingTestInputGain::kInputGainTestSet)),
        [](const auto& info) {
            auto descriptor = std::get<INPUT_GAIN_INSTANCE_NAME>(info.param).second;
            std::string gains =
                    ::android::internal::ToString(std::get<INPUT_GAIN_PARAM>(info.param));
            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               toString(descriptor.common.id.uuid) + "_inputGains_" + gains;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DynamicsProcessingTestInputGain);

/**
 * Test DynamicsProcessing Limiter Config
 */
enum LimiterConfigTestParamName {
    LIMITER_INSTANCE_NAME,
    LIMITER_CHANNEL,
    LIMITER_LINK_GROUP,
    LIMITER_ATTACK_TIME,
    LIMITER_RELEASE_TIME,
    LIMITER_RATIO,
    LIMITER_THRESHOLD,
    LIMITER_POST_GAIN,
};

using LimiterConfigTestParams = std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>,
                                           int32_t, int32_t, float, float, float, float, float>;

void fillLimiterConfig(std::vector<DynamicsProcessing::LimiterConfig>& cfg,
                       const LimiterConfigTestParams& params) {
    fillLimiterConfig(cfg, std::get<LIMITER_CHANNEL>(params), true,
                      std::get<LIMITER_LINK_GROUP>(params), std::get<LIMITER_ATTACK_TIME>(params),
                      std::get<LIMITER_RELEASE_TIME>(params), std::get<LIMITER_RATIO>(params),
                      std::get<LIMITER_THRESHOLD>(params), std::get<LIMITER_POST_GAIN>(params));
}

class DynamicsProcessingTestLimiterConfig
    : public ::testing::TestWithParam<LimiterConfigTestParams>,
      public DynamicsProcessingTestHelper {
  public:
    DynamicsProcessingTestLimiterConfig()
        : DynamicsProcessingTestHelper(std::get<LIMITER_INSTANCE_NAME>(GetParam())) {
        fillLimiterConfig(mLimiterConfigList, GetParam());
    }

    void SetUp() override { SetUpDynamicsProcessingEffect(); }

    void TearDown() override { TearDownDynamicsProcessingEffect(); }

    DynamicsProcessing::LimiterConfig mCfg;
    std::vector<DynamicsProcessing::LimiterConfig> mLimiterConfigList;
};

TEST_P(DynamicsProcessingTestLimiterConfig, SetAndGetLimiterConfig) {
    EXPECT_NO_FATAL_FAILURE(addEngineConfig(mEngineConfigPreset));
    EXPECT_NO_FATAL_FAILURE(addLimiterConfig(mLimiterConfigList));
    ASSERT_NO_FATAL_FAILURE(SetAndGetDynamicsProcessingParameters());
}

INSTANTIATE_TEST_SUITE_P(
        DynamicsProcessingTest, DynamicsProcessingTestLimiterConfig,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidDynamicsProcessing())),
                           testing::Values(-1, 0, 1, 2),  // channel index
                           testing::Values(3),            // link group
                           testing::Values(-1, 1),        // attackTime
                           testing::Values(-60, 60),      // releaseTime
                           testing::Values(-2.5, 2.5),    // ratio
                           testing::Values(-2, 2),        // thresh
                           testing::Values(-3.14, 3.14)   // postGain
                           ),
        [](const auto& info) {
            auto descriptor = std::get<LIMITER_INSTANCE_NAME>(info.param).second;
            std::vector<DynamicsProcessing::LimiterConfig> cfg;
            fillLimiterConfig(cfg, info.param);
            std::string name =
                    "Implementer_" + getPrefix(descriptor) + "_limiterConfig_" + cfg[0].toString();
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DynamicsProcessingTestLimiterConfig);

using LimiterConfigDataTestParams = std::pair<std::shared_ptr<IFactory>, Descriptor>;

class DynamicsProcessingLimiterConfigDataTest
    : public ::testing::TestWithParam<LimiterConfigDataTestParams>,
      public DynamicsProcessingTestHelper {
  public:
    DynamicsProcessingLimiterConfigDataTest()
        : DynamicsProcessingTestHelper(GetParam(), AudioChannelLayout::LAYOUT_MONO) {
        mBufferSize = kFrameCount * mChannelCount;
        mInput.resize(mBufferSize);
        generateSineWave(1000 /*Input Frequency*/, mInput);
        mInputDb = calculateDb(mInput);
    }

    void SetUp() override {
        SetUpDynamicsProcessingEffect();
        SKIP_TEST_IF_DATA_UNSUPPORTED(mDescriptor.common.flags);
    }

    void TearDown() override { TearDownDynamicsProcessingEffect(); }

    float calculateDb(std::vector<float> input, size_t start = 0) {
        return audio_utils_compute_power_mono(input.data() + start, AUDIO_FORMAT_PCM_FLOAT,
                                              input.size() - start);
    }

    void computeThreshold(float ratio, float outputDb, float& threshold) {
        EXPECT_NE(ratio, 0);
        threshold = (mInputDb - (ratio * outputDb)) / (1 - ratio);
    }

    void computeRatio(float threshold, float outputDb, float& ratio) {
        float inputOverThreshold = mInputDb - threshold;
        float outputOverThreshold = outputDb - threshold;
        EXPECT_NE(outputOverThreshold, 0);
        ratio = inputOverThreshold / outputOverThreshold;
    }

    void setParamsAndProcess(std::vector<float>& output) {
        EXPECT_NO_FATAL_FAILURE(addEngineConfig(mEngineConfigPreset));
        EXPECT_NO_FATAL_FAILURE(addLimiterConfig(mLimiterConfigList));
        ASSERT_NO_FATAL_FAILURE(SetAndGetDynamicsProcessingParameters());
        if (isAllParamsValid()) {
            ASSERT_NO_FATAL_FAILURE(
                    processAndWriteToOutput(mInput, output, mEffect, &mOpenEffectReturn));
            EXPECT_GT(output.size(), kStartIndex);
        }
        cleanUpLimiterConfig();
    }

    void cleanUpLimiterConfig() {
        CleanUp();
        mLimiterConfigList.clear();
    }
    static constexpr float kDefaultLinkerGroup = 3;
    static constexpr float kDefaultAttackTime = 0;
    static constexpr float kDefaultReleaseTime = 0;
    static constexpr float kDefaultRatio = 4;
    static constexpr float kDefaultThreshold = 0;
    static constexpr float kDefaultPostGain = 0;
    static constexpr int kInputFrequency = 1000;
    static constexpr size_t kStartIndex = 15 * kSamplingFrequency / 1000;  // skip 15ms
    std::vector<DynamicsProcessing::LimiterConfig> mLimiterConfigList;
    std::vector<float> mInput;
    float mInputDb;
    int mBufferSize;
};

TEST_P(DynamicsProcessingLimiterConfigDataTest, IncreasingThresholdDb) {
    std::vector<float> thresholdValues = {-200, -150, -100, -50, -5, 0};
    std::vector<float> output(mInput.size());
    float previousThreshold = -FLT_MAX;
    for (float threshold : thresholdValues) {
        for (int i = 0; i < mChannelCount; i++) {
            fillLimiterConfig(mLimiterConfigList, i, true, kDefaultLinkerGroup, kDefaultAttackTime,
                              kDefaultReleaseTime, kDefaultRatio, threshold, kDefaultPostGain);
        }
        EXPECT_NO_FATAL_FAILURE(setParamsAndProcess(output));
        if (!isAllParamsValid()) {
            continue;
        }
        float outputDb = calculateDb(output, kStartIndex);
        if (threshold >= mInputDb || kDefaultRatio == 1) {
            EXPECT_EQ(std::round(mInputDb), std::round(outputDb));
        } else {
            float calculatedThreshold = 0;
            EXPECT_NO_FATAL_FAILURE(computeThreshold(kDefaultRatio, outputDb, calculatedThreshold));
            ASSERT_GT(calculatedThreshold, previousThreshold);
            previousThreshold = calculatedThreshold;
        }
    }
}

TEST_P(DynamicsProcessingLimiterConfigDataTest, IncreasingRatio) {
    std::vector<float> ratioValues = {1, 10, 20, 30, 40, 50};
    std::vector<float> output(mInput.size());
    float threshold = -10;
    float previousRatio = 0;
    for (float ratio : ratioValues) {
        for (int i = 0; i < mChannelCount; i++) {
            fillLimiterConfig(mLimiterConfigList, i, true, kDefaultLinkerGroup, kDefaultAttackTime,
                              kDefaultReleaseTime, ratio, threshold, kDefaultPostGain);
        }
        EXPECT_NO_FATAL_FAILURE(setParamsAndProcess(output));
        if (!isAllParamsValid()) {
            continue;
        }
        float outputDb = calculateDb(output, kStartIndex);

        if (threshold >= mInputDb) {
            EXPECT_EQ(std::round(mInputDb), std::round(outputDb));
        } else {
            float calculatedRatio = 0;
            EXPECT_NO_FATAL_FAILURE(computeRatio(threshold, outputDb, calculatedRatio));
            ASSERT_GT(calculatedRatio, previousRatio);
            previousRatio = calculatedRatio;
        }
    }
}

TEST_P(DynamicsProcessingLimiterConfigDataTest, LimiterEnableDisable) {
    std::vector<bool> limiterEnableValues = {false, true};
    std::vector<float> output(mInput.size());
    for (bool isEnabled : limiterEnableValues) {
        for (int i = 0; i < mChannelCount; i++) {
            // Set non-default values
            fillLimiterConfig(mLimiterConfigList, i, isEnabled, kDefaultLinkerGroup,
                              5 /*attack time*/, 5 /*release time*/, 10 /*ratio*/,
                              -10 /*threshold*/, 5 /*postgain*/);
        }
        EXPECT_NO_FATAL_FAILURE(setParamsAndProcess(output));
        if (!isAllParamsValid()) {
            continue;
        }
        if (isEnabled) {
            EXPECT_NE(mInputDb, calculateDb(output, kStartIndex));
        } else {
            EXPECT_NEAR(mInputDb, calculateDb(output, kStartIndex), 0.05);
        }
    }
}

INSTANTIATE_TEST_SUITE_P(DynamicsProcessingTest, DynamicsProcessingLimiterConfigDataTest,
                         testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                 IFactory::descriptor, getEffectTypeUuidDynamicsProcessing())),
                         [](const auto& info) {
                             auto descriptor = info.param;
                             std::string name = getPrefix(descriptor.second);
                             std::replace_if(
                                     name.begin(), name.end(),
                                     [](const char c) { return !std::isalnum(c); }, '_');
                             return name;
                         });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DynamicsProcessingLimiterConfigDataTest);

/**
 * Test DynamicsProcessing ChannelConfig
 */
enum ChannelConfigTestParamName {
    BAND_CHANNEL_TEST_INSTANCE_NAME,
    BAND_CHANNEL_TEST_CHANNEL_CONFIG
};
using ChannelConfigTestParams = std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>,
                                           std::vector<DynamicsProcessing::ChannelConfig>>;

class DynamicsProcessingTestChannelConfig
    : public ::testing::TestWithParam<ChannelConfigTestParams>,
      public DynamicsProcessingTestHelper {
  public:
    DynamicsProcessingTestChannelConfig()
        : DynamicsProcessingTestHelper(std::get<BAND_CHANNEL_TEST_INSTANCE_NAME>(GetParam())),
          mCfg(std::get<BAND_CHANNEL_TEST_CHANNEL_CONFIG>(GetParam())) {}

    void SetUp() override { SetUpDynamicsProcessingEffect(); }

    void TearDown() override { TearDownDynamicsProcessingEffect(); }

    std::vector<DynamicsProcessing::ChannelConfig> mCfg;
};

TEST_P(DynamicsProcessingTestChannelConfig, SetAndGetPreEqChannelConfig) {
    EXPECT_NO_FATAL_FAILURE(addEngineConfig(mEngineConfigPreset));
    EXPECT_NO_FATAL_FAILURE(addPreEqChannelConfig(mCfg));
    ASSERT_NO_FATAL_FAILURE(SetAndGetDynamicsProcessingParameters());
}

TEST_P(DynamicsProcessingTestChannelConfig, SetAndGetPostEqChannelConfig) {
    EXPECT_NO_FATAL_FAILURE(addEngineConfig(mEngineConfigPreset));
    EXPECT_NO_FATAL_FAILURE(addPostEqChannelConfig(mCfg));
    ASSERT_NO_FATAL_FAILURE(SetAndGetDynamicsProcessingParameters());
}

TEST_P(DynamicsProcessingTestChannelConfig, SetAndGetMbcChannelConfig) {
    EXPECT_NO_FATAL_FAILURE(addEngineConfig(mEngineConfigPreset));
    EXPECT_NO_FATAL_FAILURE(addMbcChannelConfig(mCfg));
    ASSERT_NO_FATAL_FAILURE(SetAndGetDynamicsProcessingParameters());
}

INSTANTIATE_TEST_SUITE_P(
        DynamicsProcessingTest, DynamicsProcessingTestChannelConfig,
        ::testing::Combine(
                testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                        IFactory::descriptor, getEffectTypeUuidDynamicsProcessing())),
                testing::ValuesIn(
                        DynamicsProcessingTestHelper::kChannelConfigTestSet)),  // channel config
        [](const auto& info) {
            auto descriptor = std::get<BAND_CHANNEL_TEST_INSTANCE_NAME>(info.param).second;
            std::string channelConfig = ::android::internal::ToString(
                    std::get<BAND_CHANNEL_TEST_CHANNEL_CONFIG>(info.param));

            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               toString(descriptor.common.id.uuid) + "_" + channelConfig;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DynamicsProcessingTestChannelConfig);

/**
 * Test DynamicsProcessing EqBandConfig
 */
enum EqBandConfigTestParamName {
    EQ_BAND_INSTANCE_NAME,
    EQ_BAND_CHANNEL,
    EQ_BAND_CUT_OFF_FREQ,
    EQ_BAND_GAIN
};
using EqBandConfigTestParams = std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int32_t,
                                          std::vector<std::pair<int, float>>, float>;

void fillEqBandConfig(std::vector<DynamicsProcessing::EqBandConfig>& cfgs,
                      const EqBandConfigTestParams& params) {
    const std::vector<std::pair<int, float>> cutOffFreqs = std::get<EQ_BAND_CUT_OFF_FREQ>(params);
    int bandCount = cutOffFreqs.size();
    cfgs.resize(bandCount);
    for (int i = 0; i < bandCount; i++) {
        cfgs[i].channel = std::get<EQ_BAND_CHANNEL>(params);
        cfgs[i].band = cutOffFreqs[i].first;
        cfgs[i].enable = true /*Eqband Enable*/;
        cfgs[i].cutoffFrequencyHz = cutOffFreqs[i].second;
        cfgs[i].gainDb = std::get<EQ_BAND_GAIN>(params);
    }
}

class DynamicsProcessingTestEqBandConfig : public ::testing::TestWithParam<EqBandConfigTestParams>,
                                           public DynamicsProcessingTestHelper {
  public:
    DynamicsProcessingTestEqBandConfig()
        : DynamicsProcessingTestHelper(std::get<EQ_BAND_INSTANCE_NAME>(GetParam())) {
        fillEqBandConfig(mCfgs, GetParam());
    }

    void SetUp() override { SetUpDynamicsProcessingEffect(); }

    void TearDown() override { TearDownDynamicsProcessingEffect(); }

    std::vector<DynamicsProcessing::EqBandConfig> mCfgs;
};

TEST_P(DynamicsProcessingTestEqBandConfig, SetAndGetPreEqBandConfig) {
    mEngineConfigPreset.preEqStage.bandCount = mCfgs.size();
    EXPECT_NO_FATAL_FAILURE(addEngineConfig(mEngineConfigPreset));
    std::vector<DynamicsProcessing::ChannelConfig> cfgs(mChannelCount);
    for (int i = 0; i < mChannelCount; i++) {
        cfgs[i].channel = i;
        cfgs[i].enable = true;
    }
    EXPECT_NO_FATAL_FAILURE(addPreEqChannelConfig(cfgs));
    EXPECT_NO_FATAL_FAILURE(addPreEqBandConfigs(mCfgs));
    ASSERT_NO_FATAL_FAILURE(SetAndGetDynamicsProcessingParameters());
}

TEST_P(DynamicsProcessingTestEqBandConfig, SetAndGetPostEqBandConfig) {
    mEngineConfigPreset.postEqStage.bandCount = mCfgs.size();
    EXPECT_NO_FATAL_FAILURE(addEngineConfig(mEngineConfigPreset));
    std::vector<DynamicsProcessing::ChannelConfig> cfgs(mChannelCount);
    for (int i = 0; i < mChannelCount; i++) {
        cfgs[i].channel = i;
        cfgs[i].enable = true;
    }
    EXPECT_NO_FATAL_FAILURE(addPostEqChannelConfig(cfgs));
    EXPECT_NO_FATAL_FAILURE(addPostEqBandConfigs(mCfgs));
    ASSERT_NO_FATAL_FAILURE(SetAndGetDynamicsProcessingParameters());
}

std::vector<std::vector<std::pair<int, float>>> kBands{
        {
                {0, 600},
                {1, 2000},
                {2, 6000},
                {3, 10000},
                {4, 16000},
        },  // 5 bands
        {
                {0, 800},
                {3, 15000},
                {2, 6000},
                {1, 2000},
        },  // 4 bands, unsorted
        {
                {0, 650},
                {1, 2000},
                {2, 6000},
                {3, 10000},
                {3, 16000},
        },  // 5 bands, missing band
        {
                {0, 900},
                {1, 8000},
                {2, 4000},
                {3, 12000},
        },  // 4 bands, cutoff freq not increasing
        {
                {0, 450},
                {1, 2000},
                {7, 6000},
                {3, 10000},
                {4, 16000},
        },  // bad band index
        {
                {0, 1},
                {1, 8000},
        },  // too low cutoff freq
        {
                {0, 1200},
                {1, 80000},
        },  // too high cutoff freq
};

INSTANTIATE_TEST_SUITE_P(
        DynamicsProcessingTest, DynamicsProcessingTestEqBandConfig,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidDynamicsProcessing())),
                           testing::Values(-1, 0, 10),     // channel index
                           testing::ValuesIn(kBands),      // band index, cut off frequencies
                           testing::Values(-3.14f, 3.14f)  // gain
                           ),
        [](const auto& info) {
            auto descriptor = std::get<EQ_BAND_INSTANCE_NAME>(info.param).second;
            std::vector<DynamicsProcessing::EqBandConfig> cfgs;
            fillEqBandConfig(cfgs, info.param);
            std::string bands = ::android::internal::ToString(cfgs);
            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               toString(descriptor.common.id.uuid) + "_bands_" + bands;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DynamicsProcessingTestEqBandConfig);

/**
 * Test DynamicsProcessing MbcBandConfig
 */

enum MbcBandConfigParamName {
    MBC_BAND_INSTANCE_NAME,
    MBC_BAND_CHANNEL,
    MBC_BAND_CUTOFF_FREQ,
    MBC_BAND_ADDITIONAL
};
enum MbcBandConfigAdditional {
    MBC_ADD_ATTACK_TIME,
    MBC_ADD_RELEASE_TIME,
    MBC_ADD_RATIO,
    MBC_ADD_THRESHOLD,
    MBC_ADD_KNEE_WIDTH,
    MBC_ADD_NOISE_GATE_THRESHOLD,
    MBC_ADD_EXPENDER_RATIO,
    MBC_ADD_PRE_GAIN,
    MBC_ADD_POST_GAIN,
    MBC_ADD_MAX_NUM
};
using TestParamsMbcBandConfigAdditional = std::array<float, MBC_ADD_MAX_NUM>;

// attackTime, releaseTime, ratio, thresh, kneeWidth, noise, expander, preGain, postGain
static constexpr std::array<TestParamsMbcBandConfigAdditional, 4> kMbcBandConfigAdditionalParam = {
        {{-3, -10, -2, -2, -5, -90, -2.5, -2, -2},
         {0, 0, 0, 0, 0, 0, 0, 0, 0},
         {-3, 10, -2, 2, -5, 90, -2.5, 2, -2},
         {3, 10, 2, -2, -5, 90, 2.5, 2, 2}}};

using TestParamsMbcBandConfig =
        std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int32_t,
                   std::vector<std::pair<int, float>>, TestParamsMbcBandConfigAdditional>;

void fillMbcBandConfig(std::vector<DynamicsProcessing::MbcBandConfig>& cfgs,
                       const TestParamsMbcBandConfig& params) {
    const std::vector<std::pair<int, float>> cutOffFreqs = std::get<MBC_BAND_CUTOFF_FREQ>(params);
    const std::array<float, MBC_ADD_MAX_NUM> additional = std::get<MBC_BAND_ADDITIONAL>(params);
    int bandCount = cutOffFreqs.size();
    cfgs.resize(bandCount);
    for (int i = 0; i < bandCount; i++) {
        cfgs[i] = DynamicsProcessing::MbcBandConfig{
                .channel = std::get<MBC_BAND_CHANNEL>(params),
                .band = cutOffFreqs[i].first,
                .enable = true /*Mbc Band Enable*/,
                .cutoffFrequencyHz = cutOffFreqs[i].second,
                .attackTimeMs = additional[MBC_ADD_ATTACK_TIME],
                .releaseTimeMs = additional[MBC_ADD_RELEASE_TIME],
                .ratio = additional[MBC_ADD_RATIO],
                .thresholdDb = additional[MBC_ADD_THRESHOLD],
                .kneeWidthDb = additional[MBC_ADD_KNEE_WIDTH],
                .noiseGateThresholdDb = additional[MBC_ADD_NOISE_GATE_THRESHOLD],
                .expanderRatio = additional[MBC_ADD_EXPENDER_RATIO],
                .preGainDb = additional[MBC_ADD_PRE_GAIN],
                .postGainDb = additional[MBC_ADD_POST_GAIN]};
    }
}

class DynamicsProcessingTestMbcBandConfig
    : public ::testing::TestWithParam<TestParamsMbcBandConfig>,
      public DynamicsProcessingTestHelper {
  public:
    DynamicsProcessingTestMbcBandConfig()
        : DynamicsProcessingTestHelper(std::get<MBC_BAND_INSTANCE_NAME>(GetParam())) {
        fillMbcBandConfig(mCfgs, GetParam());
    }

    void SetUp() override { SetUpDynamicsProcessingEffect(); }

    void TearDown() override { TearDownDynamicsProcessingEffect(); }

    std::vector<DynamicsProcessing::MbcBandConfig> mCfgs;
};

TEST_P(DynamicsProcessingTestMbcBandConfig, SetAndGetMbcBandConfig) {
    mEngineConfigPreset.mbcStage.bandCount = mCfgs.size();
    EXPECT_NO_FATAL_FAILURE(addEngineConfig(mEngineConfigPreset));
    std::vector<DynamicsProcessing::ChannelConfig> cfgs(mChannelCount);
    for (int i = 0; i < mChannelCount; i++) {
        cfgs[i].channel = i;
        cfgs[i].enable = true;
    }
    EXPECT_NO_FATAL_FAILURE(addMbcChannelConfig(cfgs));
    EXPECT_NO_FATAL_FAILURE(addMbcBandConfigs(mCfgs));
    ASSERT_NO_FATAL_FAILURE(SetAndGetDynamicsProcessingParameters());
}

INSTANTIATE_TEST_SUITE_P(
        DynamicsProcessingTest, DynamicsProcessingTestMbcBandConfig,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidDynamicsProcessing())),
                           testing::Values(-1, 0, 10),  // channel index
                           testing::ValuesIn(kBands),   // band index, cut off frequencies
                           testing::ValuesIn(kMbcBandConfigAdditionalParam)),  // Additional
        [](const auto& info) {
            auto descriptor = std::get<MBC_BAND_INSTANCE_NAME>(info.param).second;
            std::vector<DynamicsProcessing::MbcBandConfig> cfgs;
            fillMbcBandConfig(cfgs, info.param);
            std::string mbcBands = ::android::internal::ToString(cfgs);
            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               toString(descriptor.common.id.uuid) + "_bands_" + mbcBands;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DynamicsProcessingTestMbcBandConfig);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::UnitTest::GetInstance()->listeners().Append(new TestExecutionTracer());
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
