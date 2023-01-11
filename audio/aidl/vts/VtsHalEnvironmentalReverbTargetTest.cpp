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

#define LOG_TAG "VtsHalEnvironmentalReverbTest"

#include <Utils.h>
#include <aidl/Vintf.h>
#include <unordered_set>
#include "EffectHelper.h"

using namespace android;

using aidl::android::hardware::audio::effect::Capability;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::EnvironmentalReverb;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::kEnvReverbTypeUUID;
using aidl::android::hardware::audio::effect::Parameter;

/**
 * Here we focus on specific parameter checking, general IEffect interfaces testing performed in
 * VtsAudioEffectTargetTest.
 * Testing parameter range, assuming the parameter supported by effect is in this range.
 * This range is verified with IEffect.getDescriptor() and range defined in the documentation, for
 * any index supported value test expects EX_NONE from IEffect.setParameter(), otherwise expects
 * EX_ILLEGAL_ARGUMENT.
 */

class EnvironmentalReverbHelper : public EffectHelper {
  public:
    EnvironmentalReverbHelper(std::pair<std::shared_ptr<IFactory>, Descriptor> pair) {
        std::tie(mFactory, mDescriptor) = pair;
    }

    void SetUpReverb() {
        ASSERT_NE(nullptr, mFactory);
        ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));

        Parameter::Specific specific = getDefaultParamSpecific();
        Parameter::Common common = EffectHelper::createParamCommon(
                0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */,
                kInputFrameCount /* iFrameCount */, kOutputFrameCount /* oFrameCount */);
        IEffect::OpenEffectReturn ret;
        ASSERT_NO_FATAL_FAILURE(open(mEffect, common, specific, &ret, EX_NONE));
        ASSERT_NE(nullptr, mEffect);
    }

    void TearDownReverb() {
        ASSERT_NO_FATAL_FAILURE(close(mEffect));
        ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
    }

    Parameter::Specific getDefaultParamSpecific() {
        EnvironmentalReverb er = EnvironmentalReverb::make<EnvironmentalReverb::roomLevelMb>(-6000);
        Parameter::Specific specific =
                Parameter::Specific::make<Parameter::Specific::environmentalReverb>(er);
        return specific;
    }

    static const long kInputFrameCount = 0x100, kOutputFrameCount = 0x100;
    std::shared_ptr<IFactory> mFactory;
    std::shared_ptr<IEffect> mEffect;
    Descriptor mDescriptor;
    int mRoomLevel = -6000;
    int mRoomHfLevel = 0;
    int mDecayTime = 1000;
    int mDecayHfRatio = 500;
    int mLevel = -6000;
    int mDelay = 40;
    int mDiffusion = 1000;
    int mDensity = 1000;
    bool mBypass = false;

    void SetAndGetReverbParameters() {
        for (auto& it : mTags) {
            auto& tag = it.first;
            auto& er = it.second;

            // validate parameter
            Descriptor desc;
            ASSERT_STATUS(EX_NONE, mEffect->getDescriptor(&desc));
            const bool valid = isTagInRange(it.first, it.second, desc);
            const binder_exception_t expected = valid ? EX_NONE : EX_ILLEGAL_ARGUMENT;

            // set
            Parameter expectParam;
            Parameter::Specific specific;
            specific.set<Parameter::Specific::environmentalReverb>(er);
            expectParam.set<Parameter::specific>(specific);
            EXPECT_STATUS(expected, mEffect->setParameter(expectParam)) << expectParam.toString();

            // only get if parameter in range and set success
            if (expected == EX_NONE) {
                Parameter getParam;
                Parameter::Id id;
                EnvironmentalReverb::Id erId;
                erId.set<EnvironmentalReverb::Id::commonTag>(tag);
                id.set<Parameter::Id::environmentalReverbTag>(erId);
                // if set success, then get should match
                EXPECT_STATUS(expected, mEffect->getParameter(id, &getParam));
                EXPECT_EQ(expectParam, getParam);
            }
        }
    }

    void addRoomLevelParam() {
        EnvironmentalReverb er;
        er.set<EnvironmentalReverb::roomLevelMb>(mRoomLevel);
        mTags.push_back({EnvironmentalReverb::roomLevelMb, er});
    }

    void addRoomHfLevelParam(int roomHfLevel) {
        EnvironmentalReverb er;
        er.set<EnvironmentalReverb::roomHfLevelMb>(roomHfLevel);
        mTags.push_back({EnvironmentalReverb::roomHfLevelMb, er});
    }

    void addDecayTimeParam(int decayTime) {
        EnvironmentalReverb er;
        er.set<EnvironmentalReverb::decayTimeMs>(decayTime);
        mTags.push_back({EnvironmentalReverb::decayTimeMs, er});
    }

    void addDecayHfRatioParam(int decayHfRatio) {
        EnvironmentalReverb er;
        er.set<EnvironmentalReverb::decayHfRatioPm>(decayHfRatio);
        mTags.push_back({EnvironmentalReverb::decayHfRatioPm, er});
    }

    void addLevelParam(int level) {
        EnvironmentalReverb er;
        er.set<EnvironmentalReverb::levelMb>(level);
        mTags.push_back({EnvironmentalReverb::levelMb, er});
    }

    void addDelayParam(int delay) {
        EnvironmentalReverb er;
        er.set<EnvironmentalReverb::delayMs>(delay);
        mTags.push_back({EnvironmentalReverb::delayMs, er});
    }

    void addDiffusionParam(int diffusion) {
        EnvironmentalReverb er;
        er.set<EnvironmentalReverb::diffusionPm>(diffusion);
        mTags.push_back({EnvironmentalReverb::diffusionPm, er});
    }

    void addDensityParam(int density) {
        EnvironmentalReverb er;
        er.set<EnvironmentalReverb::densityPm>(density);
        mTags.push_back({EnvironmentalReverb::densityPm, er});
    }

    void addBypassParam(bool bypass) {
        EnvironmentalReverb er;
        er.set<EnvironmentalReverb::bypass>(bypass);
        mTags.push_back({EnvironmentalReverb::bypass, er});
    }

    bool isTagInRange(const EnvironmentalReverb::Tag& tag, const EnvironmentalReverb er,
                      const Descriptor& desc) const {
        const EnvironmentalReverb::Capability& erCap =
                desc.capability.get<Capability::environmentalReverb>();
        switch (tag) {
            case EnvironmentalReverb::roomLevelMb: {
                int roomLevel = er.get<EnvironmentalReverb::roomLevelMb>();
                return isRoomLevelInRange(erCap, roomLevel);
            }
            case EnvironmentalReverb::roomHfLevelMb: {
                int roomHfLevel = er.get<EnvironmentalReverb::roomHfLevelMb>();
                return isRoomHfLevelInRange(erCap, roomHfLevel);
            }
            case EnvironmentalReverb::decayTimeMs: {
                int decayTime = er.get<EnvironmentalReverb::decayTimeMs>();
                return isDecayTimeInRange(erCap, decayTime);
            }
            case EnvironmentalReverb::decayHfRatioPm: {
                int decayHfRatio = er.get<EnvironmentalReverb::decayHfRatioPm>();
                return isDecayHfRatioInRange(erCap, decayHfRatio);
            }
            case EnvironmentalReverb::levelMb: {
                int level = er.get<EnvironmentalReverb::levelMb>();
                return isLevelInRange(erCap, level);
            }
            case EnvironmentalReverb::delayMs: {
                int delay = er.get<EnvironmentalReverb::delayMs>();
                return isDelayInRange(erCap, delay);
            }
            case EnvironmentalReverb::diffusionPm: {
                int diffusion = er.get<EnvironmentalReverb::diffusionPm>();
                return isDiffusionInRange(erCap, diffusion);
            }
            case EnvironmentalReverb::densityPm: {
                int density = er.get<EnvironmentalReverb::densityPm>();
                return isDensityInRange(erCap, density);
            }
            case EnvironmentalReverb::bypass: {
                return true;
            }
            default:
                return false;
        }
        return false;
    }

    bool isRoomLevelInRange(const EnvironmentalReverb::Capability& cap, int roomLevel) const {
        return roomLevel >= cap.minRoomLevelMb && roomLevel <= cap.maxRoomLevelMb;
    }

    bool isRoomHfLevelInRange(const EnvironmentalReverb::Capability& cap, int roomHfLevel) const {
        return roomHfLevel >= cap.minRoomHfLevelMb && roomHfLevel <= cap.maxRoomHfLevelMb;
    }

    bool isDecayTimeInRange(const EnvironmentalReverb::Capability& cap, int decayTime) const {
        return decayTime >= 0 && decayTime <= cap.maxDecayTimeMs;
    }

    bool isDecayHfRatioInRange(const EnvironmentalReverb::Capability& cap, int decayHfRatio) const {
        return decayHfRatio >= cap.minDecayHfRatioPm && decayHfRatio <= cap.maxDecayHfRatioPm;
    }

    bool isLevelInRange(const EnvironmentalReverb::Capability& cap, int level) const {
        return level >= cap.minLevelMb && level <= cap.maxLevelMb;
    }

    bool isDelayInRange(const EnvironmentalReverb::Capability& cap, int delay) const {
        return delay >= 0 && delay <= cap.maxDelayMs;
    }

    bool isDiffusionInRange(const EnvironmentalReverb::Capability& cap, int diffusion) const {
        return diffusion >= 0 && diffusion <= cap.maxDiffusionPm;
    }

    bool isDensityInRange(const EnvironmentalReverb::Capability& cap, int density) const {
        return density >= 0 && density <= cap.maxDensityPm;
    }

    static std::unordered_set<int> getRoomLevelValues() {
        auto descList = EffectFactoryHelper::getAllEffectDescriptors(IFactory::descriptor,
                                                                     kEnvReverbTypeUUID);
        int minRoomLevelMb = std::numeric_limits<int>::max();
        int maxRoomLevelMb = std::numeric_limits<int>::min();
        for (const auto& it : descList) {
            maxRoomLevelMb = std::max(
                    it.second.capability.get<Capability::environmentalReverb>().maxRoomLevelMb,
                    maxRoomLevelMb);
            minRoomLevelMb = std::min(
                    it.second.capability.get<Capability::environmentalReverb>().minRoomLevelMb,
                    minRoomLevelMb);
        }
        return {std::numeric_limits<int>::min(),        minRoomLevelMb - 1, minRoomLevelMb,
                (minRoomLevelMb + maxRoomLevelMb) >> 1, maxRoomLevelMb,     maxRoomLevelMb + 1,
                std::numeric_limits<int>::max()};
    }

    static std::unordered_set<int> getRoomHfLevelValues() {
        auto descList = EffectFactoryHelper::getAllEffectDescriptors(IFactory::descriptor,
                                                                     kEnvReverbTypeUUID);
        int minRoomHfLevelMb = std::numeric_limits<int>::max();
        int maxRoomHfLevelMb = std::numeric_limits<int>::min();
        for (const auto& it : descList) {
            maxRoomHfLevelMb = std::max(
                    it.second.capability.get<Capability::environmentalReverb>().maxRoomHfLevelMb,
                    maxRoomHfLevelMb);
            minRoomHfLevelMb = std::min(
                    it.second.capability.get<Capability::environmentalReverb>().minRoomHfLevelMb,
                    minRoomHfLevelMb);
        }
        return {std::numeric_limits<int>::min(),
                minRoomHfLevelMb - 1,
                minRoomHfLevelMb,
                (minRoomHfLevelMb + maxRoomHfLevelMb) >> 1,
                maxRoomHfLevelMb,
                maxRoomHfLevelMb + 1,
                std::numeric_limits<int>::max()};
    }

    static std::unordered_set<int> getDecayTimeValues() {
        auto descList = EffectFactoryHelper::getAllEffectDescriptors(IFactory::descriptor,
                                                                     kEnvReverbTypeUUID);
        const auto max = std::max_element(
                descList.begin(), descList.end(),
                [](const std::pair<std::shared_ptr<IFactory>, Descriptor>& a,
                   const std::pair<std::shared_ptr<IFactory>, Descriptor>& b) {
                    return a.second.capability.get<Capability::environmentalReverb>()
                                   .maxDecayTimeMs <
                           b.second.capability.get<Capability::environmentalReverb>()
                                   .maxDecayTimeMs;
                });
        if (max == descList.end()) {
            return {0};
        }
        int maxDecayTimeMs =
                max->second.capability.get<Capability::environmentalReverb>().maxDecayTimeMs;
        return {-1, 0, maxDecayTimeMs >> 1, maxDecayTimeMs - 1, maxDecayTimeMs, maxDecayTimeMs + 1};
    }

    static std::unordered_set<int> getDecayHfRatioValues() {
        auto descList = EffectFactoryHelper::getAllEffectDescriptors(IFactory::descriptor,
                                                                     kEnvReverbTypeUUID);
        int minDecayHfRatioPm = std::numeric_limits<int>::max();
        int maxDecayHfRatioPm = std::numeric_limits<int>::min();
        for (const auto& it : descList) {
            maxDecayHfRatioPm = std::max(
                    it.second.capability.get<Capability::environmentalReverb>().maxDecayHfRatioPm,
                    maxDecayHfRatioPm);
            minDecayHfRatioPm = std::min(
                    it.second.capability.get<Capability::environmentalReverb>().minDecayHfRatioPm,
                    minDecayHfRatioPm);
        }
        return {std::numeric_limits<int>::min(),
                minDecayHfRatioPm - 1,
                minDecayHfRatioPm,
                (minDecayHfRatioPm + maxDecayHfRatioPm) >> 1,
                maxDecayHfRatioPm,
                maxDecayHfRatioPm + 1,
                std::numeric_limits<int>::max()};
    }

    static std::unordered_set<int> getLevelValues() {
        auto descList = EffectFactoryHelper::getAllEffectDescriptors(IFactory::descriptor,
                                                                     kEnvReverbTypeUUID);
        int minLevelMb = std::numeric_limits<int>::max();
        int maxLevelMb = std::numeric_limits<int>::min();
        for (const auto& it : descList) {
            maxLevelMb =
                    std::max(it.second.capability.get<Capability::environmentalReverb>().maxLevelMb,
                             maxLevelMb);
            minLevelMb =
                    std::min(it.second.capability.get<Capability::environmentalReverb>().minLevelMb,
                             minLevelMb);
        }
        return {std::numeric_limits<int>::min(), minLevelMb - 1, minLevelMb,
                (minLevelMb + maxLevelMb) >> 1,  maxLevelMb,     maxLevelMb + 1,
                std::numeric_limits<int>::max()};
    }

    static std::unordered_set<int> getDelayValues() {
        auto descList = EffectFactoryHelper::getAllEffectDescriptors(IFactory::descriptor,
                                                                     kEnvReverbTypeUUID);
        const auto max = std::max_element(
                descList.begin(), descList.end(),
                [](const std::pair<std::shared_ptr<IFactory>, Descriptor>& a,
                   const std::pair<std::shared_ptr<IFactory>, Descriptor>& b) {
                    return a.second.capability.get<Capability::environmentalReverb>().maxDelayMs <
                           b.second.capability.get<Capability::environmentalReverb>().maxDelayMs;
                });
        if (max == descList.end()) {
            return {0};
        }
        int maxDelayMs = max->second.capability.get<Capability::environmentalReverb>().maxDelayMs;
        return {-1, 0, maxDelayMs >> 1, maxDelayMs - 1, maxDelayMs, maxDelayMs + 1};
    }

    static std::unordered_set<int> getDiffusionValues() {
        auto descList = EffectFactoryHelper::getAllEffectDescriptors(IFactory::descriptor,
                                                                     kEnvReverbTypeUUID);
        const auto max = std::max_element(
                descList.begin(), descList.end(),
                [](const std::pair<std::shared_ptr<IFactory>, Descriptor>& a,
                   const std::pair<std::shared_ptr<IFactory>, Descriptor>& b) {
                    return a.second.capability.get<Capability::environmentalReverb>()
                                   .maxDiffusionPm <
                           b.second.capability.get<Capability::environmentalReverb>()
                                   .maxDiffusionPm;
                });
        if (max == descList.end()) {
            return {0};
        }
        int maxDiffusionPm =
                max->second.capability.get<Capability::environmentalReverb>().maxDiffusionPm;
        return {-1, 0, maxDiffusionPm >> 1, maxDiffusionPm - 1, maxDiffusionPm, maxDiffusionPm + 1};
    }

    static std::unordered_set<int> getDensityValues() {
        auto descList = EffectFactoryHelper::getAllEffectDescriptors(IFactory::descriptor,
                                                                     kEnvReverbTypeUUID);
        const auto max = std::max_element(
                descList.begin(), descList.end(),
                [](const std::pair<std::shared_ptr<IFactory>, Descriptor>& a,
                   const std::pair<std::shared_ptr<IFactory>, Descriptor>& b) {
                    return a.second.capability.get<Capability::environmentalReverb>().maxDensityPm <
                           b.second.capability.get<Capability::environmentalReverb>().maxDensityPm;
                });
        if (max == descList.end()) {
            return {0};
        }
        int maxDensityPm =
                max->second.capability.get<Capability::environmentalReverb>().maxDensityPm;
        return {-1, 0, maxDensityPm >> 1, maxDensityPm - 1, maxDensityPm, maxDensityPm + 1};
    }

  private:
    std::vector<std::pair<EnvironmentalReverb::Tag, EnvironmentalReverb>> mTags;
    void CleanUp() { mTags.clear(); }
};

class EnvironmentalReverbRoomLevelTest
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int>>,
      public EnvironmentalReverbHelper {
  public:
    EnvironmentalReverbRoomLevelTest() : EnvironmentalReverbHelper(std::get<0>(GetParam())) {
        mRoomLevel = std::get<1>(GetParam());
    }

    void SetUp() override { SetUpReverb(); }

    void TearDown() override { TearDownReverb(); }
};

TEST_P(EnvironmentalReverbRoomLevelTest, SetAndGetRoomLevel) {
    EXPECT_NO_FATAL_FAILURE(addRoomLevelParam());
    SetAndGetReverbParameters();
}

INSTANTIATE_TEST_SUITE_P(
        EnvironmentalReverbTest, EnvironmentalReverbRoomLevelTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, kEnvReverbTypeUUID)),
                           testing::ValuesIn(EnvironmentalReverbHelper::getRoomLevelValues())),
        [](const testing::TestParamInfo<EnvironmentalReverbRoomLevelTest::ParamType>& info) {
            auto descriptor = std::get<0>(info.param).second;
            std::string roomLevel = std::to_string(std::get<1>(info.param));

            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               descriptor.common.id.uuid.toString() + "_roomLevel" + roomLevel;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EnvironmentalReverbRoomLevelTest);

class EnvironmentalReverbRoomHfLevelTest
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int>>,
      public EnvironmentalReverbHelper {
  public:
    EnvironmentalReverbRoomHfLevelTest() : EnvironmentalReverbHelper(std::get<0>(GetParam())) {
        mRoomHfLevel = std::get<1>(GetParam());
    }

    void SetUp() override { SetUpReverb(); }

    void TearDown() override { TearDownReverb(); }
};

TEST_P(EnvironmentalReverbRoomHfLevelTest, SetAndGetRoomHfLevel) {
    EXPECT_NO_FATAL_FAILURE(addRoomHfLevelParam(mRoomHfLevel));
    SetAndGetReverbParameters();
}

INSTANTIATE_TEST_SUITE_P(
        EnvironmentalReverbTest, EnvironmentalReverbRoomHfLevelTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, kEnvReverbTypeUUID)),
                           testing::ValuesIn(EnvironmentalReverbHelper::getRoomHfLevelValues())),
        [](const testing::TestParamInfo<EnvironmentalReverbRoomHfLevelTest::ParamType>& info) {
            auto descriptor = std::get<0>(info.param).second;
            std::string roomHfLevel = std::to_string(std::get<1>(info.param));

            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               descriptor.common.id.uuid.toString() + "_roomHfLevel" + roomHfLevel;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EnvironmentalReverbRoomHfLevelTest);

class EnvironmentalReverbDecayTimeTest
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int>>,
      public EnvironmentalReverbHelper {
  public:
    EnvironmentalReverbDecayTimeTest() : EnvironmentalReverbHelper(std::get<0>(GetParam())) {
        mDecayTime = std::get<1>(GetParam());
    }

    void SetUp() override { SetUpReverb(); }

    void TearDown() override { TearDownReverb(); }
};

TEST_P(EnvironmentalReverbDecayTimeTest, SetAndGetDecayTime) {
    EXPECT_NO_FATAL_FAILURE(addDecayTimeParam(mDecayTime));
    SetAndGetReverbParameters();
}

INSTANTIATE_TEST_SUITE_P(
        EnvironmentalReverbTest, EnvironmentalReverbDecayTimeTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, kEnvReverbTypeUUID)),
                           testing::ValuesIn(EnvironmentalReverbHelper::getDecayTimeValues())),
        [](const testing::TestParamInfo<EnvironmentalReverbDecayTimeTest::ParamType>& info) {
            auto descriptor = std::get<0>(info.param).second;
            std::string decayTime = std::to_string(std::get<1>(info.param));

            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               descriptor.common.id.uuid.toString() + "_decayTime" + decayTime;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EnvironmentalReverbDecayTimeTest);

class EnvironmentalReverbDecayHfRatioTest
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int>>,
      public EnvironmentalReverbHelper {
  public:
    EnvironmentalReverbDecayHfRatioTest() : EnvironmentalReverbHelper(std::get<0>(GetParam())) {
        mDecayHfRatio = std::get<1>(GetParam());
    }

    void SetUp() override { SetUpReverb(); }

    void TearDown() override { TearDownReverb(); }
};

TEST_P(EnvironmentalReverbDecayHfRatioTest, SetAndGetDecayHfRatio) {
    EXPECT_NO_FATAL_FAILURE(addDecayHfRatioParam(mDecayHfRatio));
    SetAndGetReverbParameters();
}

INSTANTIATE_TEST_SUITE_P(
        EnvironmentalReverbTest, EnvironmentalReverbDecayHfRatioTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, kEnvReverbTypeUUID)),
                           testing::ValuesIn(EnvironmentalReverbHelper::getDecayHfRatioValues())),
        [](const testing::TestParamInfo<EnvironmentalReverbDecayHfRatioTest::ParamType>& info) {
            auto descriptor = std::get<0>(info.param).second;
            std::string decayHfRatio = std::to_string(std::get<1>(info.param));

            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               descriptor.common.id.uuid.toString() + "_decayHfRatio" +
                               decayHfRatio;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EnvironmentalReverbDecayHfRatioTest);

class EnvironmentalReverbLevelTest
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int>>,
      public EnvironmentalReverbHelper {
  public:
    EnvironmentalReverbLevelTest() : EnvironmentalReverbHelper(std::get<0>(GetParam())) {
        mLevel = std::get<1>(GetParam());
    }

    void SetUp() override { SetUpReverb(); }

    void TearDown() override { TearDownReverb(); }
};

TEST_P(EnvironmentalReverbLevelTest, SetAndGetLevel) {
    EXPECT_NO_FATAL_FAILURE(addLevelParam(mLevel));
    SetAndGetReverbParameters();
}

INSTANTIATE_TEST_SUITE_P(
        EnvironmentalReverbTest, EnvironmentalReverbLevelTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, kEnvReverbTypeUUID)),
                           testing::ValuesIn(EnvironmentalReverbHelper::getLevelValues())),
        [](const testing::TestParamInfo<EnvironmentalReverbDecayHfRatioTest::ParamType>& info) {
            auto descriptor = std::get<0>(info.param).second;
            std::string level = std::to_string(std::get<1>(info.param));

            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               descriptor.common.id.uuid.toString() + "_level" + level;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EnvironmentalReverbLevelTest);

class EnvironmentalReverbDelayTest
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int>>,
      public EnvironmentalReverbHelper {
  public:
    EnvironmentalReverbDelayTest() : EnvironmentalReverbHelper(std::get<0>(GetParam())) {
        mDelay = std::get<1>(GetParam());
    }

    void SetUp() override { SetUpReverb(); }

    void TearDown() override { TearDownReverb(); }
};

TEST_P(EnvironmentalReverbDelayTest, SetAndGetDelay) {
    EXPECT_NO_FATAL_FAILURE(addDelayParam(mDelay));
    SetAndGetReverbParameters();
}

INSTANTIATE_TEST_SUITE_P(
        EnvironmentalReverbTest, EnvironmentalReverbDelayTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, kEnvReverbTypeUUID)),
                           testing::ValuesIn(EnvironmentalReverbHelper::getDelayValues())),
        [](const testing::TestParamInfo<EnvironmentalReverbDelayTest::ParamType>& info) {
            auto descriptor = std::get<0>(info.param).second;
            std::string delay = std::to_string(std::get<1>(info.param));

            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               descriptor.common.id.uuid.toString() + "_delay" + delay;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EnvironmentalReverbDelayTest);

class EnvironmentalReverbDiffusionTest
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int>>,
      public EnvironmentalReverbHelper {
  public:
    EnvironmentalReverbDiffusionTest() : EnvironmentalReverbHelper(std::get<0>(GetParam())) {
        mDiffusion = std::get<1>(GetParam());
    }

    void SetUp() override { SetUpReverb(); }

    void TearDown() override { TearDownReverb(); }
};

TEST_P(EnvironmentalReverbDiffusionTest, SetAndGetDiffusion) {
    EXPECT_NO_FATAL_FAILURE(addDiffusionParam(mDiffusion));
    SetAndGetReverbParameters();
}

INSTANTIATE_TEST_SUITE_P(
        EnvironmentalReverbTest, EnvironmentalReverbDiffusionTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, kEnvReverbTypeUUID)),
                           testing::ValuesIn(EnvironmentalReverbHelper::getDiffusionValues())),
        [](const testing::TestParamInfo<EnvironmentalReverbDiffusionTest::ParamType>& info) {
            auto descriptor = std::get<0>(info.param).second;
            std::string diffusion = std::to_string(std::get<1>(info.param));

            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               descriptor.common.id.uuid.toString() + "_diffusion" + diffusion;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EnvironmentalReverbDiffusionTest);

class EnvironmentalReverbDensityTest
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int>>,
      public EnvironmentalReverbHelper {
  public:
    EnvironmentalReverbDensityTest() : EnvironmentalReverbHelper(std::get<0>(GetParam())) {
        mDensity = std::get<1>(GetParam());
    }

    void SetUp() override { SetUpReverb(); }

    void TearDown() override { TearDownReverb(); }
};

TEST_P(EnvironmentalReverbDensityTest, SetAndGetDensity) {
    EXPECT_NO_FATAL_FAILURE(addDensityParam(mDensity));
    SetAndGetReverbParameters();
}

INSTANTIATE_TEST_SUITE_P(
        EnvironmentalReverbTest, EnvironmentalReverbDensityTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, kEnvReverbTypeUUID)),
                           testing::ValuesIn(EnvironmentalReverbHelper::getDensityValues())),
        [](const testing::TestParamInfo<EnvironmentalReverbDensityTest::ParamType>& info) {
            auto descriptor = std::get<0>(info.param).second;
            std::string density = std::to_string(std::get<1>(info.param));

            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               descriptor.common.id.uuid.toString() + "_density" + density;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EnvironmentalReverbDensityTest);

class EnvironmentalReverbBypassTest
    : public ::testing::TestWithParam<
              std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, bool>>,
      public EnvironmentalReverbHelper {
  public:
    EnvironmentalReverbBypassTest() : EnvironmentalReverbHelper(std::get<0>(GetParam())) {
        mBypass = std::get<1>(GetParam());
    }

    void SetUp() override { SetUpReverb(); }

    void TearDown() override { TearDownReverb(); }
};

TEST_P(EnvironmentalReverbBypassTest, SetAndGetBypass) {
    EXPECT_NO_FATAL_FAILURE(addBypassParam(mBypass));
    SetAndGetReverbParameters();
}

INSTANTIATE_TEST_SUITE_P(
        EnvironmentalReverbTest, EnvironmentalReverbBypassTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, kEnvReverbTypeUUID)),
                           testing::Bool()),
        [](const testing::TestParamInfo<EnvironmentalReverbBypassTest::ParamType>& info) {
            auto descriptor = std::get<0>(info.param).second;
            std::string bypass = std::to_string(std::get<1>(info.param));

            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               descriptor.common.id.uuid.toString() + "_bypass" + bypass;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EnvironmentalReverbBypassTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
