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

#include <aidl/Vintf.h>
#define LOG_TAG "VtsHalAGC1ParamTest"
#include <android-base/logging.h>

#include "EffectHelper.h"

using namespace android;

using aidl::android::hardware::audio::effect::AutomaticGainControlV1;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::getEffectTypeUuidAutomaticGainControlV1;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;

enum ParamName {
    PARAM_INSTANCE_NAME,
    PARAM_TARGET_PEAK_LEVEL,
    PARAM_MAX_COMPRESSION_GAIN,
    PARAM_ENABLE_LIMITER
};
using AGC1ParamTestParam =
        std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int /* targetPeakLevel */,
                   int /* maxCompressionGain */, bool /* enableLimiter */>;

class AGC1ParamTest : public ::testing::TestWithParam<AGC1ParamTestParam>, public EffectHelper {
  public:
    AGC1ParamTest()
        : mTargetPeakLevel(std::get<PARAM_TARGET_PEAK_LEVEL>(GetParam())),
          mMaxCompressionGain(std::get<PARAM_MAX_COMPRESSION_GAIN>(GetParam())),
          mEnableLimiter(std::get<PARAM_ENABLE_LIMITER>(GetParam())) {
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
        AutomaticGainControlV1 AGC1 =
                AutomaticGainControlV1::make<AutomaticGainControlV1::targetPeakLevelDbFs>(0);
        Parameter::Specific specific =
                Parameter::Specific::make<Parameter::Specific::automaticGainControlV1>(AGC1);
        return specific;
    }

    static const long kInputFrameCount = 0x100, kOutputFrameCount = 0x100;
    std::shared_ptr<IFactory> mFactory;
    std::shared_ptr<IEffect> mEffect;
    Descriptor mDescriptor;
    int mTargetPeakLevel;
    int mMaxCompressionGain;
    bool mEnableLimiter;

    void SetAndGetParameters() {
        for (auto& it : mTags) {
            auto& tag = it.first;
            auto& AGC1 = it.second;

            // validate parameter
            Descriptor desc;
            ASSERT_STATUS(EX_NONE, mEffect->getDescriptor(&desc));
            const bool valid =
                    isParameterValid<AutomaticGainControlV1, Range::automaticGainControlV1>(AGC1,
                                                                                            desc);
            const binder_exception_t expected = valid ? EX_NONE : EX_ILLEGAL_ARGUMENT;

            // set parameter
            Parameter expectParam;
            Parameter::Specific specific;
            specific.set<Parameter::Specific::automaticGainControlV1>(AGC1);
            expectParam.set<Parameter::specific>(specific);
            EXPECT_STATUS(expected, mEffect->setParameter(expectParam)) << expectParam.toString();

            // only get if parameter in range and set success
            if (expected == EX_NONE) {
                Parameter getParam;
                Parameter::Id id;
                AutomaticGainControlV1::Id specificId;
                specificId.set<AutomaticGainControlV1::Id::commonTag>(tag);
                id.set<Parameter::Id::automaticGainControlV1Tag>(specificId);
                EXPECT_STATUS(EX_NONE, mEffect->getParameter(id, &getParam));

                EXPECT_EQ(expectParam, getParam) << "\nexpect:" << expectParam.toString()
                                                 << "\ngetParam:" << getParam.toString();
            }
        }
    }

    void addTargetPeakLevelParam(int targetPeakLevel) {
        AutomaticGainControlV1 AGC1;
        AGC1.set<AutomaticGainControlV1::targetPeakLevelDbFs>(targetPeakLevel);
        mTags.push_back({AutomaticGainControlV1::targetPeakLevelDbFs, AGC1});
    }
    void addMaxCompressionGainParam(int maxCompressionGainDb) {
        AutomaticGainControlV1 AGC1;
        AGC1.set<AutomaticGainControlV1::maxCompressionGainDb>(maxCompressionGainDb);
        mTags.push_back({AutomaticGainControlV1::maxCompressionGainDb, AGC1});
    }
    void addEnableLimiterParam(bool enableLimiter) {
        AutomaticGainControlV1 AGC1;
        AGC1.set<AutomaticGainControlV1::enableLimiter>(enableLimiter);
        mTags.push_back({AutomaticGainControlV1::enableLimiter, AGC1});
    }

  private:
    std::vector<std::pair<AutomaticGainControlV1::Tag, AutomaticGainControlV1>> mTags;
    void CleanUp() { mTags.clear(); }
};

TEST_P(AGC1ParamTest, SetAndGetTargetPeakLevelParam) {
    EXPECT_NO_FATAL_FAILURE(addTargetPeakLevelParam(mTargetPeakLevel));
    SetAndGetParameters();
}

TEST_P(AGC1ParamTest, SetAndGetMaxCompressionGain) {
    EXPECT_NO_FATAL_FAILURE(addMaxCompressionGainParam(mMaxCompressionGain));
    SetAndGetParameters();
}

TEST_P(AGC1ParamTest, SetAndGetEnableLimiter) {
    EXPECT_NO_FATAL_FAILURE(addEnableLimiterParam(mEnableLimiter));
    SetAndGetParameters();
}

std::vector<std::pair<std::shared_ptr<IFactory>, Descriptor>> kDescPair;
INSTANTIATE_TEST_SUITE_P(
        AGC1ParamTest, AGC1ParamTest,
        ::testing::Combine(
                testing::ValuesIn(kDescPair = EffectFactoryHelper::getAllEffectDescriptors(
                                          IFactory::descriptor,
                                          getEffectTypeUuidAutomaticGainControlV1())),
                testing::ValuesIn(EffectHelper::getTestValueSet<
                                  AutomaticGainControlV1, int, Range::automaticGainControlV1,
                                  AutomaticGainControlV1::targetPeakLevelDbFs>(
                        kDescPair, EffectHelper::expandTestValueBasic<int>)),
                testing::ValuesIn(EffectHelper::getTestValueSet<
                                  AutomaticGainControlV1, int, Range::automaticGainControlV1,
                                  AutomaticGainControlV1::maxCompressionGainDb>(
                        kDescPair, EffectHelper::expandTestValueBasic<int>)),
                testing::Bool()),
        [](const testing::TestParamInfo<AGC1ParamTest::ParamType>& info) {
            auto descriptor = std::get<PARAM_INSTANCE_NAME>(info.param).second;
            std::string targetPeakLevel =
                    std::to_string(std::get<PARAM_TARGET_PEAK_LEVEL>(info.param));
            std::string maxCompressionGain =
                    std::to_string(std::get<PARAM_MAX_COMPRESSION_GAIN>(info.param));
            std::string enableLimiter = std::to_string(std::get<PARAM_ENABLE_LIMITER>(info.param));

            std::string name = getPrefix(descriptor) + "_target_peak_level_" + targetPeakLevel +
                               "_max_compression_gain_" + maxCompressionGain + "_enable_limiter_" +
                               enableLimiter;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AGC1ParamTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
