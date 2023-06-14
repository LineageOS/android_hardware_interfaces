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
#define LOG_TAG "VtsHalPresetReverbTargetTest"
#include <android-base/logging.h>
#include <android/binder_enums.h>

#include "EffectHelper.h"

using namespace android;

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::getEffectTypeUuidPresetReverb;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;
using aidl::android::hardware::audio::effect::PresetReverb;

/**
 * Here we focus on specific parameter checking, general IEffect interfaces testing performed in
 * VtsAudioEffectTargetTest.
 */
enum ParamName { PARAM_INSTANCE_NAME, PARAM_PRESETS };
using PresetReverbParamTestParam =
        std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, PresetReverb::Presets>;

// Testing for enum values
const std::vector<PresetReverb::Presets> kPresetsValues{
        ndk::enum_range<PresetReverb::Presets>().begin(),
        ndk::enum_range<PresetReverb::Presets>().end()};

class PresetReverbParamTest : public ::testing::TestWithParam<PresetReverbParamTestParam>,
                              public EffectHelper {
  public:
    PresetReverbParamTest() : mParamPresets(std::get<PARAM_PRESETS>(GetParam())) {
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

    static const long kInputFrameCount = 0x100, kOutputFrameCount = 0x100;
    std::shared_ptr<IFactory> mFactory;
    std::shared_ptr<IEffect> mEffect;
    Descriptor mDescriptor;
    PresetReverb::Presets mParamPresets = PresetReverb::Presets::NONE;

    void SetAndGetPresetReverbParameters() {
        for (auto& it : mTags) {
            auto& tag = it.first;
            auto& pr = it.second;

            // validate parameter
            Descriptor desc;
            ASSERT_STATUS(EX_NONE, mEffect->getDescriptor(&desc));
            const bool valid = isParameterValid<PresetReverb, Range::presetReverb>(it.second, desc);
            const binder_exception_t expected = valid ? EX_NONE : EX_ILLEGAL_ARGUMENT;

            // set parameter
            Parameter expectParam;
            Parameter::Specific specific;
            specific.set<Parameter::Specific::presetReverb>(pr);
            expectParam.set<Parameter::specific>(specific);
            // All values are valid, set parameter should succeed
            EXPECT_STATUS(expected, mEffect->setParameter(expectParam)) << expectParam.toString();

            // get parameter
            Parameter getParam;
            Parameter::Id id;
            PresetReverb::Id prId;
            prId.set<PresetReverb::Id::commonTag>(tag);
            id.set<Parameter::Id::presetReverbTag>(prId);
            EXPECT_STATUS(expected, mEffect->getParameter(id, &getParam));

            EXPECT_EQ(expectParam, getParam);
        }
    }

    void addPresetsParam(PresetReverb::Presets preset) {
        PresetReverb pr;
        pr.set<PresetReverb::preset>(preset);
        mTags.push_back({PresetReverb::preset, pr});
    }

    Parameter::Specific getDefaultParamSpecific() {
        PresetReverb pr = PresetReverb::make<PresetReverb::preset>(PresetReverb::Presets::NONE);
        Parameter::Specific specific =
                Parameter::Specific::make<Parameter::Specific::presetReverb>(pr);
        return specific;
    }

  private:
    std::vector<std::pair<PresetReverb::Tag, PresetReverb>> mTags;
    void CleanUp() { mTags.clear(); }
};

TEST_P(PresetReverbParamTest, SetAndGetPresets) {
    EXPECT_NO_FATAL_FAILURE(addPresetsParam(mParamPresets));
    SetAndGetPresetReverbParameters();
}

INSTANTIATE_TEST_SUITE_P(
        PresetReverbTest, PresetReverbParamTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidPresetReverb())),
                           testing::ValuesIn(kPresetsValues)),
        [](const testing::TestParamInfo<PresetReverbParamTest::ParamType>& info) {
            auto descriptor = std::get<PARAM_INSTANCE_NAME>(info.param).second;
            std::string preset =
                    std::to_string(static_cast<int>(std::get<PARAM_PRESETS>(info.param)));
            std::string name = getPrefix(descriptor) + "_preset" + preset;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(PresetReverbParamTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
