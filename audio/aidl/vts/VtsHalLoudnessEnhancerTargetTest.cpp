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

#define LOG_TAG "VtsHalLoudnessEnhancerTest"

#include <Utils.h>
#include "EffectHelper.h"

using namespace android;

using aidl::android::hardware::audio::effect::Capability;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::LoudnessEnhancer;
using aidl::android::hardware::audio::effect::LoudnessEnhancerTypeUUID;
using aidl::android::hardware::audio::effect::Parameter;

/**
 * Here we focus on specific parameter checking, general IEffect interfaces testing performed in
 * VtsAudioEffectTargetTest.
 */
enum ParamName { PARAM_INSTANCE_NAME, PARAM_GAIN_MB };
using LoudnessEnhancerParamTestParam = std::tuple<std::string, int>;

// Every int 32 bit value is a valid gain, so testing the corner cases and one regular value.
// TODO : Update the test values once range/capability is updated by implementation.
const std::vector<int> kGainMbValues = {std::numeric_limits<int>::min(), 100,
                                        std::numeric_limits<int>::max()};

class LoudnessEnhancerParamTest : public ::testing::TestWithParam<LoudnessEnhancerParamTestParam>,
                                  public EffectHelper {
  public:
    LoudnessEnhancerParamTest()
        : EffectHelper(std::get<PARAM_INSTANCE_NAME>(GetParam())),
          mParamGainMb(std::get<PARAM_GAIN_MB>(GetParam())) {}

    void SetUp() override {
        CreateEffectsWithUUID(LoudnessEnhancerTypeUUID);
        initParamCommonFormat();
        initParamCommon();
        initParamSpecific();
        OpenEffects(LoudnessEnhancerTypeUUID);
        SCOPED_TRACE(testing::Message() << "gainMb: " << mParamGainMb);
    }

    void TearDown() override {
        CloseEffects();
        DestroyEffects();
        CleanUp();
    }

    int mParamGainMb = 0;

    void SetAndGetLoudnessEnhancerParameters() {
        auto functor = [&](const std::shared_ptr<IEffect>& effect) {
            for (auto& it : mTags) {
                auto& tag = it.first;
                auto& le = it.second;

                // set parameter
                Parameter expectParam;
                Parameter::Specific specific;
                specific.set<Parameter::Specific::loudnessEnhancer>(le);
                expectParam.set<Parameter::specific>(specific);
                // All values are valid, set parameter should succeed
                EXPECT_STATUS(EX_NONE, effect->setParameter(expectParam)) << expectParam.toString();

                // get parameter
                Parameter getParam;
                Parameter::Id id;
                LoudnessEnhancer::Id leId;
                leId.set<LoudnessEnhancer::Id::commonTag>(tag);
                id.set<Parameter::Id::loudnessEnhancerTag>(leId);
                EXPECT_STATUS(EX_NONE, effect->getParameter(id, &getParam));

                EXPECT_EQ(expectParam, getParam);
            }
        };
        EXPECT_NO_FATAL_FAILURE(ForEachEffect(functor));
    }

    void addGainMbParam(int gainMb) {
        LoudnessEnhancer le;
        le.set<LoudnessEnhancer::gainMb>(gainMb);
        mTags.push_back({LoudnessEnhancer::gainMb, le});
    }

  private:
    std::vector<std::pair<LoudnessEnhancer::Tag, LoudnessEnhancer>> mTags;

    void initParamSpecific() {
        LoudnessEnhancer le;
        le.set<LoudnessEnhancer::gainMb>(0);
        Parameter::Specific specific;
        specific.set<Parameter::Specific::loudnessEnhancer>(le);
        setSpecific(specific);
    }

    void CleanUp() { mTags.clear(); }
};

TEST_P(LoudnessEnhancerParamTest, SetAndGetGainMb) {
    EXPECT_NO_FATAL_FAILURE(addGainMbParam(mParamGainMb));
    SetAndGetLoudnessEnhancerParameters();
}

INSTANTIATE_TEST_SUITE_P(
        LoudnessEnhancerTest, LoudnessEnhancerParamTest,
        ::testing::Combine(
                testing::ValuesIn(android::getAidlHalInstanceNames(IFactory::descriptor)),
                testing::ValuesIn(kGainMbValues)),
        [](const testing::TestParamInfo<LoudnessEnhancerParamTest::ParamType>& info) {
            std::string instance = std::get<PARAM_INSTANCE_NAME>(info.param);
            std::string gainMb = std::to_string(std::get<PARAM_GAIN_MB>(info.param));

            std::string name = instance + "_gainMb" + gainMb;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(LoudnessEnhancerParamTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
