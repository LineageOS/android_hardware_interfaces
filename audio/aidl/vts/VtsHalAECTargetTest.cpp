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

#include <algorithm>
#include <string>
#include <unordered_set>

#include <aidl/Vintf.h>
#define LOG_TAG "VtsHalAECParamTest"
#include <android-base/logging.h>

#include "EffectHelper.h"
#include "effect-impl/EffectTypes.h"

using namespace android;

using aidl::android::hardware::audio::effect::AcousticEchoCanceler;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::getEffectTypeUuidAcousticEchoCanceler;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;
using aidl::android::hardware::audio::effect::Range;

enum ParamName { PARAM_INSTANCE_NAME, PARAM_ECHO_DELAY, PARAM_MOBILE_MODE };
using AECParamTestParam = std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>,
                                     int /* echoDelayUs */, bool /* mobileMode */>;

class AECParamTest : public ::testing::TestWithParam<AECParamTestParam>, public EffectHelper {
  public:
    AECParamTest()
        : mEchoDelay(std::get<PARAM_ECHO_DELAY>(GetParam())),
          mMobileMode(std::get<PARAM_MOBILE_MODE>(GetParam())) {
        std::tie(mFactory, mDescriptor) = std::get<PARAM_INSTANCE_NAME>(GetParam());
    }

    void SetUp() override {
        ASSERT_NE(nullptr, mFactory);
        ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));

        auto specific = getDefaultParamSpecific();
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

    std::optional<Parameter::Specific> getDefaultParamSpecific() {
        auto aec = AcousticEchoCanceler::make<AcousticEchoCanceler::echoDelayUs>(0);
        if (!isParameterValid<AcousticEchoCanceler, Range::acousticEchoCanceler>(aec,
                                                                                 mDescriptor)) {
            return std::nullopt;
        }

        Parameter::Specific specific =
                Parameter::Specific::make<Parameter::Specific::acousticEchoCanceler>(aec);
        return specific;
    }

    static const long kInputFrameCount = 0x100, kOutputFrameCount = 0x100;
    std::shared_ptr<IFactory> mFactory;
    std::shared_ptr<IEffect> mEffect;
    Descriptor mDescriptor;

    int mEchoDelay;
    bool mMobileMode;

    void SetAndGetParameters() {
        for (auto& it : mTags) {
            auto& tag = it.first;
            auto& aec = it.second;

            // validate parameter
            Descriptor desc;
            ASSERT_STATUS(EX_NONE, mEffect->getDescriptor(&desc));
            const bool valid =
                    isParameterValid<AcousticEchoCanceler, Range::acousticEchoCanceler>(aec, desc);
            const binder_exception_t expected = valid ? EX_NONE : EX_ILLEGAL_ARGUMENT;

            // set parameter
            Parameter expectParam;
            Parameter::Specific specific;
            specific.set<Parameter::Specific::acousticEchoCanceler>(aec);
            expectParam.set<Parameter::specific>(specific);
            EXPECT_STATUS(expected, mEffect->setParameter(expectParam)) << expectParam.toString();

            // only get if parameter in range and set success
            if (expected == EX_NONE) {
                Parameter getParam;
                Parameter::Id id;
                AcousticEchoCanceler::Id specificId;
                specificId.set<AcousticEchoCanceler::Id::commonTag>(tag);
                id.set<Parameter::Id::acousticEchoCancelerTag>(specificId);
                EXPECT_STATUS(EX_NONE, mEffect->getParameter(id, &getParam));

                EXPECT_EQ(expectParam, getParam) << "\nexpect:" << expectParam.toString()
                                                 << "\ngetParam:" << getParam.toString();
            }
        }
    }

    void addEchoDelayParam(int delay) {
        AcousticEchoCanceler aec;
        aec.set<AcousticEchoCanceler::echoDelayUs>(delay);
        mTags.push_back({AcousticEchoCanceler::echoDelayUs, aec});
    }

    void addMobileModeParam(bool mode) {
        AcousticEchoCanceler aec;
        aec.set<AcousticEchoCanceler::mobileMode>(mode);
        mTags.push_back({AcousticEchoCanceler::mobileMode, aec});
    }

    static std::unordered_set<bool> getMobileModeValues() { return {true, false}; }

  private:
    std::vector<std::pair<AcousticEchoCanceler::Tag, AcousticEchoCanceler>> mTags;
    void CleanUp() { mTags.clear(); }
};

TEST_P(AECParamTest, SetAndGetEchoDelay) {
    EXPECT_NO_FATAL_FAILURE(addEchoDelayParam(mEchoDelay));
    SetAndGetParameters();
}

TEST_P(AECParamTest, SetAndGetMobileMode) {
    EXPECT_NO_FATAL_FAILURE(addMobileModeParam(mMobileMode));
    SetAndGetParameters();
}

std::vector<std::pair<std::shared_ptr<IFactory>, Descriptor>> kDescPair;
INSTANTIATE_TEST_SUITE_P(
        AECParamTest, AECParamTest,
        ::testing::Combine(
                testing::ValuesIn(kDescPair = EffectFactoryHelper::getAllEffectDescriptors(
                                          IFactory::descriptor,
                                          getEffectTypeUuidAcousticEchoCanceler())),
                testing::ValuesIn(EffectHelper::getTestValueSet<AcousticEchoCanceler, int,
                                                                Range::acousticEchoCanceler,
                                                                AcousticEchoCanceler::echoDelayUs>(
                        kDescPair, EffectHelper::expandTestValueBasic<int>)),
                testing::ValuesIn(EffectHelper::getTestValueSet<AcousticEchoCanceler, bool,
                                                                Range::acousticEchoCanceler,
                                                                AcousticEchoCanceler::mobileMode>(
                        kDescPair, EffectHelper::expandTestValueBasic<bool>))),
        [](const testing::TestParamInfo<AECParamTest::ParamType>& info) {
            auto descriptor = std::get<PARAM_INSTANCE_NAME>(info.param).second;
            std::string echoDelay = std::to_string(std::get<PARAM_ECHO_DELAY>(info.param));
            std::string mobileMode = std::get<PARAM_MOBILE_MODE>(info.param) ? "true" : "false";
            std::string name =
                    getPrefix(descriptor) + "_EchoDelay_" + echoDelay + "_MobileMode_" + mobileMode;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AECParamTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
