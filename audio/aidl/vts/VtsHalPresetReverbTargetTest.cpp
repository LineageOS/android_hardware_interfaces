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

#define LOG_TAG "VtsHalPresetReverbTargetTest"
#include <android-base/logging.h>
#include <android/binder_enums.h>
#include <audio_utils/power.h>
#include <system/audio.h>

#include "EffectHelper.h"

using namespace android;

using aidl::android::hardware::audio::common::getChannelCount;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::getEffectTypeUuidPresetReverb;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;
using aidl::android::hardware::audio::effect::PresetReverb;
using android::hardware::audio::common::testing::detail::TestExecutionTracer;

class PresetReverbHelper : public EffectHelper {
  public:
    void SetUpPresetReverb() {
        ASSERT_NE(nullptr, mFactory);
        ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
        Parameter::Specific specific = getDefaultParamSpecific();
        Parameter::Common common = createParamCommon(
                0 /* session */, 1 /* ioHandle */, kSamplingFrequency /* iSampleRate */,
                kSamplingFrequency /* oSampleRate */, mFrameCount /* iFrameCount */,
                mFrameCount /* oFrameCount */);
        ASSERT_NO_FATAL_FAILURE(open(mEffect, common, specific, &mOpenEffectReturn, EX_NONE));
        ASSERT_NE(nullptr, mEffect);
    }

    void TearDownPresetReverb() {
        ASSERT_NO_FATAL_FAILURE(close(mEffect));
        ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
        mOpenEffectReturn = IEffect::OpenEffectReturn{};
    }

    Parameter::Specific getDefaultParamSpecific() {
        PresetReverb pr = PresetReverb::make<PresetReverb::preset>(kDefaultPreset);
        Parameter::Specific specific =
                Parameter::Specific::make<Parameter::Specific::presetReverb>(pr);
        return specific;
    }

    Parameter createPresetReverbParam(const PresetReverb::Presets& param) {
        return Parameter::make<Parameter::specific>(
                Parameter::Specific::make<Parameter::Specific::presetReverb>(
                        PresetReverb::make<PresetReverb::preset>(param)));
    }

    void setAndVerifyPreset(const PresetReverb::Presets& param) {
        auto expectedParam = createPresetReverbParam(param);
        EXPECT_STATUS(EX_NONE, mEffect->setParameter(expectedParam)) << expectedParam.toString();

        PresetReverb::Id revId =
                PresetReverb::Id::make<PresetReverb::Id::commonTag>(PresetReverb::preset);

        auto id = Parameter::Id::make<Parameter::Id::presetReverbTag>(revId);
        // get parameter
        Parameter getParam;
        EXPECT_STATUS(EX_NONE, mEffect->getParameter(id, &getParam));
        EXPECT_EQ(expectedParam, getParam) << "\nexpectedParam:" << expectedParam.toString()
                                           << "\ngetParam:" << getParam.toString();
    }

    static constexpr int kSamplingFrequency = 44100;
    static constexpr int kDurationMilliSec = 500;
    static constexpr int kBufferSize = kSamplingFrequency * kDurationMilliSec / 1000;
    int mStereoChannelCount =
            getChannelCount(AudioChannelLayout::make<AudioChannelLayout::layoutMask>(
                    AudioChannelLayout::LAYOUT_STEREO));
    PresetReverb::Presets kDefaultPreset = PresetReverb::Presets::NONE;
    int mFrameCount = kBufferSize / mStereoChannelCount;
    std::shared_ptr<IFactory> mFactory;
    std::shared_ptr<IEffect> mEffect;
    IEffect::OpenEffectReturn mOpenEffectReturn;
    Descriptor mDescriptor;
};

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
                              public PresetReverbHelper {
  public:
    PresetReverbParamTest() : mParamPreset(std::get<PARAM_PRESETS>(GetParam())) {
        std::tie(mFactory, mDescriptor) = std::get<PARAM_INSTANCE_NAME>(GetParam());
    }

    void SetUp() override { ASSERT_NO_FATAL_FAILURE(SetUpPresetReverb()); }

    void TearDown() override { TearDownPresetReverb(); }

    const PresetReverb::Presets mParamPreset;
};

TEST_P(PresetReverbParamTest, SetAndGetPresets) {
    ASSERT_NO_FATAL_FAILURE(setAndVerifyPreset(mParamPreset));
}

using PresetReverbProcessTestParam = std::pair<std::shared_ptr<IFactory>, Descriptor>;

class PresetReverbProcessTest : public ::testing::TestWithParam<PresetReverbProcessTestParam>,
                                public PresetReverbHelper {
  public:
    PresetReverbProcessTest() {
        std::tie(mFactory, mDescriptor) = GetParam();
        generateSineWaveInput();
    }

    void SetUp() override {
        SKIP_TEST_IF_DATA_UNSUPPORTED(mDescriptor.common.flags);
        ASSERT_NO_FATAL_FAILURE(SetUpPresetReverb());
    }
    void TearDown() override {
        SKIP_TEST_IF_DATA_UNSUPPORTED(mDescriptor.common.flags);
        ASSERT_NO_FATAL_FAILURE(TearDownPresetReverb());
    }

    void generateSineWaveInput() {
        int frequency = 1000;
        for (size_t i = 0; i < kBufferSize; i++) {
            mInput.push_back(sin(2 * M_PI * frequency * i / kSamplingFrequency));
        }
    }

    bool isAuxiliary() {
        return mDescriptor.common.flags.type ==
               aidl::android::hardware::audio::effect::Flags::Type::AUXILIARY;
    }

    float computeReverbOutputEnergy(std::vector<float> output) {
        if (!isAuxiliary()) {
            // Extract auxiliary output
            for (size_t i = 0; i < output.size(); i++) {
                output[i] -= mInput[i];
            }
        }
        return (audio_utils_compute_energy_mono(output.data(), AUDIO_FORMAT_PCM_FLOAT,
                                                output.size()));
    }

    void setPresetAndProcess(const PresetReverb::Presets& preset, std::vector<float>& output) {
        ASSERT_NO_FATAL_FAILURE(setAndVerifyPreset(preset));
        ASSERT_NO_FATAL_FAILURE(
                processAndWriteToOutput(mInput, output, mEffect, &mOpenEffectReturn));
    }

    void validateIncreasingEnergy(const std::vector<PresetReverb::Presets>& presets) {
        float baseOutputEnergy = 0;

        for (PresetReverb::Presets preset : presets) {
            std::vector<float> output(kBufferSize);
            setPresetAndProcess(preset, output);
            float outputEnergy = computeReverbOutputEnergy(output);

            ASSERT_GT(outputEnergy, baseOutputEnergy);
            baseOutputEnergy = outputEnergy;
        }
    }

    std::vector<float> mInput;
};

TEST_P(PresetReverbProcessTest, DecreasingRoomSize) {
    std::vector<PresetReverb::Presets> roomPresets = {PresetReverb::Presets::LARGEROOM,
                                                      PresetReverb::Presets::MEDIUMROOM,
                                                      PresetReverb::Presets::SMALLROOM};
    validateIncreasingEnergy(roomPresets);
}

TEST_P(PresetReverbProcessTest, DecreasingHallSize) {
    std::vector<PresetReverb::Presets> hallPresets = {PresetReverb::Presets::LARGEHALL,
                                                      PresetReverb::Presets::MEDIUMHALL};
    validateIncreasingEnergy(hallPresets);
}

TEST_P(PresetReverbProcessTest, PresetPlate) {
    std::vector<float> output(kBufferSize);

    setPresetAndProcess(PresetReverb::Presets::PLATE, output);
    float outputEnergy = computeReverbOutputEnergy(output);
    // Since there is no comparator preset, validating it is greater than zero
    ASSERT_GT(outputEnergy, 0);
}

TEST_P(PresetReverbProcessTest, PresetNone) {
    std::vector<float> output(kBufferSize);

    setPresetAndProcess(kDefaultPreset, output);
    float outputEnergy = computeReverbOutputEnergy(output);
    // NONE type doesn't create reverb effect
    ASSERT_EQ(outputEnergy, 0);
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

INSTANTIATE_TEST_SUITE_P(
        PresetReverbTest, PresetReverbProcessTest,
        testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                IFactory::descriptor, getEffectTypeUuidPresetReverb())),
        [](const testing::TestParamInfo<PresetReverbProcessTest::ParamType>& info) {
            auto descriptor = info.param;
            return getPrefix(descriptor.second);
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(PresetReverbProcessTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::UnitTest::GetInstance()->listeners().Append(new TestExecutionTracer());
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
