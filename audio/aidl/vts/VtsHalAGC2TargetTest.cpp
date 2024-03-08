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
#define LOG_TAG "VtsHalAGC2ParamTest"
#include <android-base/logging.h>
#include <android/binder_enums.h>

#include "EffectHelper.h"

using namespace android;

using aidl::android::hardware::audio::effect::AutomaticGainControlV2;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::getEffectTypeUuidAutomaticGainControlV2;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;
using android::hardware::audio::common::testing::detail::TestExecutionTracer;

enum ParamName {
    PARAM_INSTANCE_NAME,
    PARAM_DIGITAL_GAIN,
    PARAM_SATURATION_MARGIN,
    PARAM_LEVEL_ESTIMATOR
};
using AGC2ParamTestParam =
        std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int /* gain */,
                   int /* margin */, AutomaticGainControlV2::LevelEstimator>;

class AGC2ParamTest : public ::testing::TestWithParam<AGC2ParamTestParam>, public EffectHelper {
  public:
    AGC2ParamTest()
        : mGain(std::get<PARAM_DIGITAL_GAIN>(GetParam())),
          mMargin(std::get<PARAM_SATURATION_MARGIN>(GetParam())),
          mLevelEstimator(std::get<PARAM_LEVEL_ESTIMATOR>(GetParam())) {
        std::tie(mFactory, mDescriptor) = std::get<PARAM_INSTANCE_NAME>(GetParam());
    }

    void SetUp() override {
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

    void TearDown() override {
        ASSERT_NO_FATAL_FAILURE(close(mEffect));
        ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
    }

    Parameter::Specific getDefaultParamSpecific() {
        AutomaticGainControlV2 AGC2 =
                AutomaticGainControlV2::make<AutomaticGainControlV2::fixedDigitalGainMb>(0);
        Parameter::Specific specific =
                Parameter::Specific::make<Parameter::Specific::automaticGainControlV2>(AGC2);
        return specific;
    }

    static const long kInputFrameCount = 0x100, kOutputFrameCount = 0x100;
    std::shared_ptr<IFactory> mFactory;
    std::shared_ptr<IEffect> mEffect;
    Descriptor mDescriptor;
    int mGain;
    int mMargin;
    AutomaticGainControlV2::LevelEstimator mLevelEstimator;

    void SetAndGetParameters() {
        for (auto& it : mTags) {
            auto& tag = it.first;
            auto& AGC2 = it.second;

            // validate parameter
            Descriptor desc;
            ASSERT_STATUS(EX_NONE, mEffect->getDescriptor(&desc));
            const bool valid =
                    isParameterValid<AutomaticGainControlV2, Range::automaticGainControlV2>(AGC2,
                                                                                            desc);
            const binder_exception_t expected = valid ? EX_NONE : EX_ILLEGAL_ARGUMENT;

            // set parameter
            Parameter expectParam;
            Parameter::Specific specific;
            specific.set<Parameter::Specific::automaticGainControlV2>(AGC2);
            expectParam.set<Parameter::specific>(specific);
            EXPECT_STATUS(expected, mEffect->setParameter(expectParam)) << expectParam.toString();

            // only get if parameter in range and set success
            if (expected == EX_NONE) {
                Parameter getParam;
                Parameter::Id id;
                AutomaticGainControlV2::Id specificId;
                specificId.set<AutomaticGainControlV2::Id::commonTag>(tag);
                id.set<Parameter::Id::automaticGainControlV2Tag>(specificId);
                EXPECT_STATUS(EX_NONE, mEffect->getParameter(id, &getParam));

                EXPECT_EQ(expectParam, getParam) << "\nexpect:" << expectParam.toString()
                                                 << "\ngetParam:" << getParam.toString();
            }
        }
    }

    void addDigitalGainParam(int gain) {
        AutomaticGainControlV2 AGC2;
        AGC2.set<AutomaticGainControlV2::fixedDigitalGainMb>(gain);
        mTags.push_back({AutomaticGainControlV2::fixedDigitalGainMb, AGC2});
    }
    void addSaturationMarginParam(int margin) {
        AutomaticGainControlV2 AGC2;
        AGC2.set<AutomaticGainControlV2::saturationMarginMb>(margin);
        mTags.push_back({AutomaticGainControlV2::saturationMarginMb, AGC2});
    }
    void addLevelEstimatorParam(AutomaticGainControlV2::LevelEstimator levelEstimator) {
        AutomaticGainControlV2 AGC2;
        AGC2.set<AutomaticGainControlV2::levelEstimator>(levelEstimator);
        mTags.push_back({AutomaticGainControlV2::levelEstimator, AGC2});
    }

    static std::set<AutomaticGainControlV2::LevelEstimator> getLevelEstimatorValues() {
        return {ndk::enum_range<AutomaticGainControlV2::LevelEstimator>().begin(),
                ndk::enum_range<AutomaticGainControlV2::LevelEstimator>().end()};
    }

  private:
    std::vector<std::pair<AutomaticGainControlV2::Tag, AutomaticGainControlV2>> mTags;
    void CleanUp() { mTags.clear(); }
};

TEST_P(AGC2ParamTest, SetAndGetDigitalGainParam) {
    EXPECT_NO_FATAL_FAILURE(addDigitalGainParam(mGain));
    SetAndGetParameters();
}

TEST_P(AGC2ParamTest, SetAndGetSaturationMargin) {
    EXPECT_NO_FATAL_FAILURE(addSaturationMarginParam(mMargin));
    SetAndGetParameters();
}

TEST_P(AGC2ParamTest, SetAndGetLevelEstimator) {
    EXPECT_NO_FATAL_FAILURE(addLevelEstimatorParam(mLevelEstimator));
    SetAndGetParameters();
}

std::vector<std::pair<std::shared_ptr<IFactory>, Descriptor>> kDescPair;
INSTANTIATE_TEST_SUITE_P(
        AGC2ParamTest, AGC2ParamTest,
        ::testing::Combine(
                testing::ValuesIn(kDescPair = EffectFactoryHelper::getAllEffectDescriptors(
                                          IFactory::descriptor,
                                          getEffectTypeUuidAutomaticGainControlV2())),
                testing::ValuesIn(EffectHelper::getTestValueSet<
                                  AutomaticGainControlV2, int, Range::automaticGainControlV2,
                                  AutomaticGainControlV2::fixedDigitalGainMb>(
                        kDescPair, EffectHelper::expandTestValueBasic<int>)),
                testing::ValuesIn(EffectHelper::getTestValueSet<
                                  AutomaticGainControlV2, int, Range::automaticGainControlV2,
                                  AutomaticGainControlV2::saturationMarginMb>(
                        kDescPair, EffectHelper::expandTestValueBasic<int>)),
                testing::ValuesIn(AGC2ParamTest::getLevelEstimatorValues())),
        [](const testing::TestParamInfo<AGC2ParamTest::ParamType>& info) {
            auto descriptor = std::get<PARAM_INSTANCE_NAME>(info.param).second;
            std::string gain = std::to_string(std::get<PARAM_DIGITAL_GAIN>(info.param));
            std::string estimator = aidl::android::hardware::audio::effect::toString(
                    std::get<PARAM_LEVEL_ESTIMATOR>(info.param));
            std::string margin =
                    std::to_string(static_cast<int>(std::get<PARAM_SATURATION_MARGIN>(info.param)));

            std::string name = getPrefix(descriptor) + "_digital_gain_" + gain +
                               "_level_estimator_" + estimator + "_margin_" + margin;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AGC2ParamTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::UnitTest::GetInstance()->listeners().Append(new TestExecutionTracer());
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
