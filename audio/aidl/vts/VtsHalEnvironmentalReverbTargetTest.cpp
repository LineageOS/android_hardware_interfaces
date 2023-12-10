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

#include <aidl/Vintf.h>
#define LOG_TAG "VtsHalEnvironmentalReverbTest"
#include <android-base/logging.h>

#include "EffectHelper.h"

using namespace android;

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::EnvironmentalReverb;
using aidl::android::hardware::audio::effect::getEffectTypeUuidEnvReverb;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
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
            const bool valid = isParameterValid<EnvironmentalReverb, Range::environmentalReverb>(
                    it.second, desc);
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

std::vector<std::pair<std::shared_ptr<IFactory>, Descriptor>> kDescPair;

INSTANTIATE_TEST_SUITE_P(
        EnvironmentalReverbTest, EnvironmentalReverbRoomLevelTest,
        ::testing::Combine(
                testing::ValuesIn(kDescPair = EffectFactoryHelper::getAllEffectDescriptors(
                                          IFactory::descriptor, getEffectTypeUuidEnvReverb())),
                testing::ValuesIn(EffectHelper::getTestValueSet<EnvironmentalReverb, int,
                                                                Range::environmentalReverb,
                                                                EnvironmentalReverb::roomLevelMb>(
                        kDescPair, EffectHelper::expandTestValueBasic<int>))),
        [](const testing::TestParamInfo<EnvironmentalReverbRoomLevelTest::ParamType>& info) {
            auto descriptor = std::get<0>(info.param).second;
            std::string roomLevel = std::to_string(std::get<1>(info.param));

            std::string name = getPrefix(descriptor) + "_roomLevel" + roomLevel;
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
                                   IFactory::descriptor, getEffectTypeUuidEnvReverb())),
                           testing::ValuesIn(EffectHelper::getTestValueSet<
                                             EnvironmentalReverb, int, Range::environmentalReverb,
                                             EnvironmentalReverb::roomHfLevelMb>(
                                   kDescPair, EffectHelper::expandTestValueBasic<int>))),
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
                                   IFactory::descriptor, getEffectTypeUuidEnvReverb())),
                           testing::ValuesIn(EffectHelper::getTestValueSet<
                                             EnvironmentalReverb, int, Range::environmentalReverb,
                                             EnvironmentalReverb::decayTimeMs>(
                                   kDescPair, EffectHelper::expandTestValueBasic<int>))),
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
                                   IFactory::descriptor, getEffectTypeUuidEnvReverb())),
                           testing::ValuesIn(EffectHelper::getTestValueSet<
                                             EnvironmentalReverb, int, Range::environmentalReverb,
                                             EnvironmentalReverb::decayHfRatioPm>(
                                   kDescPair, EffectHelper::expandTestValueBasic<int>))),
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
                                   IFactory::descriptor, getEffectTypeUuidEnvReverb())),
                           testing::ValuesIn(EffectHelper::getTestValueSet<
                                             EnvironmentalReverb, int, Range::environmentalReverb,
                                             EnvironmentalReverb::levelMb>(
                                   kDescPair, EffectHelper::expandTestValueBasic<int>))),
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
                                   IFactory::descriptor, getEffectTypeUuidEnvReverb())),
                           testing::ValuesIn(EffectHelper::getTestValueSet<
                                             EnvironmentalReverb, int, Range::environmentalReverb,
                                             EnvironmentalReverb::delayMs>(
                                   kDescPair, EffectHelper::expandTestValueBasic<int>))),
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
                                   IFactory::descriptor, getEffectTypeUuidEnvReverb())),
                           testing::ValuesIn(EffectHelper::getTestValueSet<
                                             EnvironmentalReverb, int, Range::environmentalReverb,
                                             EnvironmentalReverb::diffusionPm>(
                                   kDescPair, EffectHelper::expandTestValueBasic<int>))),
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
                                   IFactory::descriptor, getEffectTypeUuidEnvReverb())),
                           testing::ValuesIn(EffectHelper::getTestValueSet<
                                             EnvironmentalReverb, int, Range::environmentalReverb,
                                             EnvironmentalReverb::densityPm>(
                                   kDescPair, EffectHelper::expandTestValueBasic<int>))),
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
                                   IFactory::descriptor, getEffectTypeUuidEnvReverb())),
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
