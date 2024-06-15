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

#define LOG_TAG "VtsHalVolumeTest"
#include <android-base/logging.h>

#include "EffectHelper.h"

using namespace android;

using aidl::android::hardware::audio::common::getChannelCount;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::getEffectTypeUuidVolume;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;
using aidl::android::hardware::audio::effect::Volume;
using android::hardware::audio::common::testing::detail::TestExecutionTracer;

class VolumeControlHelper : public EffectHelper {
  public:
    void SetUpVolumeControl() {
        ASSERT_NE(nullptr, mFactory);
        ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
        initFrameCount();
        Parameter::Specific specific = getDefaultParamSpecific();
        Parameter::Common common = EffectHelper::createParamCommon(
                0 /* session */, 1 /* ioHandle */, kSamplingFrequency /* iSampleRate */,
                kSamplingFrequency /* oSampleRate */, mInputFrameCount /* iFrameCount */,
                mInputFrameCount /* oFrameCount */);
        ASSERT_NO_FATAL_FAILURE(open(mEffect, common, specific, &mOpenEffectReturn, EX_NONE));
        ASSERT_NE(nullptr, mEffect);
    }

    void TearDownVolumeControl() {
        ASSERT_NO_FATAL_FAILURE(close(mEffect));
        ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
        mOpenEffectReturn = IEffect::OpenEffectReturn{};
    }

    Parameter::Specific getDefaultParamSpecific() {
        Volume vol = Volume::make<Volume::levelDb>(kMinLevel);
        Parameter::Specific specific = Parameter::Specific::make<Parameter::Specific::volume>(vol);
        return specific;
    }

    Parameter createVolumeParam(int param, Volume::Tag volTag) {
        return Parameter::make<Parameter::specific>(
                Parameter::Specific::make<Parameter::Specific::volume>(
                        (volTag == Volume::mute) ? Volume::make<Volume::mute>(param)
                                                 : Volume::make<Volume::levelDb>(param)));
    }

    void initFrameCount() {
        int channelCount = getChannelCount(
                AudioChannelLayout::make<AudioChannelLayout::layoutMask>(kDefaultChannelLayout));
        mInputFrameCount = kBufferSize / channelCount;
        mOutputFrameCount = kBufferSize / channelCount;
    }

    bool isLevelValid(int level) {
        auto vol = Volume::make<Volume::levelDb>(level);
        return isParameterValid<Volume, Range::volume>(vol, mDescriptor);
    }

    void setAndVerifyParameters(Volume::Tag volTag, int param, binder_exception_t expected) {
        auto expectedParam = createVolumeParam(param, volTag);
        EXPECT_STATUS(expected, mEffect->setParameter(expectedParam)) << expectedParam.toString();

        if (expected == EX_NONE) {
            Volume::Id volId = Volume::Id::make<Volume::Id::commonTag>(volTag);

            auto id = Parameter::Id::make<Parameter::Id::volumeTag>(volId);
            // get parameter
            Parameter getParam;
            // if set success, then get should match
            EXPECT_STATUS(expected, mEffect->getParameter(id, &getParam));
            EXPECT_EQ(expectedParam, getParam) << "\nexpectedParam:" << expectedParam.toString()
                                               << "\ngetParam:" << getParam.toString();
        }
    }

    static constexpr int kSamplingFrequency = 44100;
    static constexpr int kDurationMilliSec = 2000;
    static constexpr int kBufferSize = kSamplingFrequency * kDurationMilliSec / 1000;
    static constexpr int kMinLevel = -96;
    static constexpr int kDefaultChannelLayout = AudioChannelLayout::LAYOUT_STEREO;
    long mInputFrameCount, mOutputFrameCount;
    std::shared_ptr<IFactory> mFactory;
    std::shared_ptr<IEffect> mEffect;
    IEffect::OpenEffectReturn mOpenEffectReturn;
    Descriptor mDescriptor;
};
/**
 * Here we focus on specific parameter checking, general IEffect interfaces testing performed in
 * VtsAudioEffectTargetTest.
 */
enum ParamName { PARAM_INSTANCE_NAME, PARAM_LEVEL, PARAM_MUTE };
using VolumeParamTestParam =
        std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int, bool>;

class VolumeParamTest : public ::testing::TestWithParam<VolumeParamTestParam>,
                        public VolumeControlHelper {
  public:
    VolumeParamTest()
        : mParamLevel(std::get<PARAM_LEVEL>(GetParam())),
          mParamMute(std::get<PARAM_MUTE>(GetParam())) {
        std::tie(mFactory, mDescriptor) = std::get<PARAM_INSTANCE_NAME>(GetParam());
    }

    void SetUp() override { ASSERT_NO_FATAL_FAILURE(SetUpVolumeControl()); }
    void TearDown() override { TearDownVolumeControl(); }

    int mParamLevel = 0;
    bool mParamMute = false;
};

TEST_P(VolumeParamTest, SetAndGetParams) {
    ASSERT_NO_FATAL_FAILURE(
            setAndVerifyParameters(Volume::levelDb, mParamLevel,
                                   isLevelValid(mParamLevel) ? EX_NONE : EX_ILLEGAL_ARGUMENT));
    ASSERT_NO_FATAL_FAILURE(setAndVerifyParameters(Volume::mute, mParamMute, EX_NONE));
}

using VolumeDataTestParam = std::pair<std::shared_ptr<IFactory>, Descriptor>;

class VolumeDataTest : public ::testing::TestWithParam<VolumeDataTestParam>,
                       public VolumeControlHelper {
  public:
    VolumeDataTest() {
        std::tie(mFactory, mDescriptor) = GetParam();
        mInput.resize(kBufferSize);
        mInputMag.resize(mTestFrequencies.size());
        mBinOffsets.resize(mTestFrequencies.size());
        roundToFreqCenteredToFftBin(mTestFrequencies, mBinOffsets, kBinWidth);
        generateMultiTone(mTestFrequencies, mInput, kSamplingFrequency);
        mInputMag = calculateMagnitude(mInput, mBinOffsets, kNPointFFT);
    }

    std::vector<int> calculatePercentageDiff(const std::vector<float>& outputMag) {
        std::vector<int> percentages(mTestFrequencies.size());

        for (size_t i = 0; i < mInputMag.size(); i++) {
            float diff = mInputMag[i] - outputMag[i];
            percentages[i] = std::round(diff / mInputMag[i] * 100);
        }
        return percentages;
    }

    // Convert Decibel value to Percentage
    int percentageDb(float level) { return std::round((1 - (pow(10, level / 20))) * 100); }

    void SetUp() override {
        SKIP_TEST_IF_DATA_UNSUPPORTED(mDescriptor.common.flags);
        ASSERT_NO_FATAL_FAILURE(SetUpVolumeControl());
    }
    void TearDown() override {
        SKIP_TEST_IF_DATA_UNSUPPORTED(mDescriptor.common.flags);
        TearDownVolumeControl();
    }

    static constexpr int kMaxAudioSample = 1;
    static constexpr int kTransitionDuration = 300;
    static constexpr int kNPointFFT = 32768;
    static constexpr float kBinWidth = (float)kSamplingFrequency / kNPointFFT;
    static constexpr size_t offset = kSamplingFrequency * kTransitionDuration / 1000;
    static constexpr float kBaseLevel = 0;
    std::vector<int> mTestFrequencies = {100, 1000};
    std::vector<float> mInput;
    std::vector<float> mInputMag;
    std::vector<int> mBinOffsets;
};

TEST_P(VolumeDataTest, ApplyLevelMuteUnmute) {
    std::vector<float> output(kBufferSize);
    std::vector<int> diffs(mTestFrequencies.size());
    std::vector<float> outputMag(mTestFrequencies.size());

    if (!isLevelValid(kBaseLevel)) {
        GTEST_SKIP() << "Volume Level not supported, skipping the test\n";
    }

    // Apply Volume Level

    ASSERT_NO_FATAL_FAILURE(setAndVerifyParameters(Volume::levelDb, kBaseLevel, EX_NONE));
    ASSERT_NO_FATAL_FAILURE(processAndWriteToOutput(mInput, output, mEffect, &mOpenEffectReturn));

    outputMag = calculateMagnitude(output, mBinOffsets, kNPointFFT);
    diffs = calculatePercentageDiff(outputMag);

    for (size_t i = 0; i < diffs.size(); i++) {
        ASSERT_EQ(diffs[i], percentageDb(kBaseLevel));
    }

    // Apply Mute

    ASSERT_NO_FATAL_FAILURE(setAndVerifyParameters(Volume::mute, true /*mute*/, EX_NONE));
    ASSERT_NO_FATAL_FAILURE(processAndWriteToOutput(mInput, output, mEffect, &mOpenEffectReturn));

    std::vector<float> subOutputMute(output.begin() + offset, output.end());
    outputMag = calculateMagnitude(subOutputMute, mBinOffsets, kNPointFFT);
    diffs = calculatePercentageDiff(outputMag);

    for (size_t i = 0; i < diffs.size(); i++) {
        ASSERT_EQ(diffs[i], percentageDb(kMinLevel /*Mute*/));
    }

    // Verifying Fade out
    outputMag = calculateMagnitude(output, mBinOffsets, kNPointFFT);
    diffs = calculatePercentageDiff(outputMag);

    for (size_t i = 0; i < diffs.size(); i++) {
        ASSERT_LT(diffs[i], percentageDb(kMinLevel /*Mute*/));
    }

    // Apply Unmute

    ASSERT_NO_FATAL_FAILURE(setAndVerifyParameters(Volume::mute, false /*unmute*/, EX_NONE));
    ASSERT_NO_FATAL_FAILURE(processAndWriteToOutput(mInput, output, mEffect, &mOpenEffectReturn));

    std::vector<float> subOutputUnmute(output.begin() + offset, output.end());

    outputMag = calculateMagnitude(subOutputUnmute, mBinOffsets, kNPointFFT);
    diffs = calculatePercentageDiff(outputMag);

    for (size_t i = 0; i < diffs.size(); i++) {
        ASSERT_EQ(diffs[i], percentageDb(kBaseLevel));
    }

    // Verifying Fade in
    outputMag = calculateMagnitude(output, mBinOffsets, kNPointFFT);
    diffs = calculatePercentageDiff(outputMag);

    for (size_t i = 0; i < diffs.size(); i++) {
        ASSERT_GT(diffs[i], percentageDb(kBaseLevel));
    }
}

TEST_P(VolumeDataTest, DecreasingLevels) {
    std::vector<int> decreasingLevels = {-24, -48, -96};
    std::vector<float> baseOutput(kBufferSize);
    std::vector<int> baseDiffs(mTestFrequencies.size());
    std::vector<float> outputMag(mTestFrequencies.size());

    if (!isLevelValid(kBaseLevel)) {
        GTEST_SKIP() << "Volume Level not supported, skipping the test\n";
    }

    ASSERT_NO_FATAL_FAILURE(setAndVerifyParameters(Volume::levelDb, kBaseLevel, EX_NONE));
    ASSERT_NO_FATAL_FAILURE(
            processAndWriteToOutput(mInput, baseOutput, mEffect, &mOpenEffectReturn));

    outputMag = calculateMagnitude(baseOutput, mBinOffsets, kNPointFFT);
    baseDiffs = calculatePercentageDiff(outputMag);

    for (int level : decreasingLevels) {
        std::vector<float> output(kBufferSize);
        std::vector<int> diffs(mTestFrequencies.size());

        // Skipping the further steps for unnsupported level values
        if (!isLevelValid(level)) {
            continue;
        }
        ASSERT_NO_FATAL_FAILURE(setAndVerifyParameters(Volume::levelDb, level, EX_NONE));
        ASSERT_NO_FATAL_FAILURE(
                processAndWriteToOutput(mInput, output, mEffect, &mOpenEffectReturn));

        outputMag = calculateMagnitude(output, mBinOffsets, kNPointFFT);
        diffs = calculatePercentageDiff(outputMag);

        // Decrease in volume level results in greater magnitude difference
        for (size_t i = 0; i < diffs.size(); i++) {
            ASSERT_GT(diffs[i], baseDiffs[i]);
        }

        baseDiffs = diffs;
    }
}

std::vector<std::pair<std::shared_ptr<IFactory>, Descriptor>> kDescPair;
INSTANTIATE_TEST_SUITE_P(
        VolumeTest, VolumeParamTest,
        ::testing::Combine(
                testing::ValuesIn(kDescPair = EffectFactoryHelper::getAllEffectDescriptors(
                                          IFactory::descriptor, getEffectTypeUuidVolume())),
                testing::ValuesIn(
                        EffectHelper::getTestValueSet<Volume, int, Range::volume, Volume::levelDb>(
                                kDescPair, EffectHelper::expandTestValueBasic<int>)),
                testing::Bool() /* mute */),
        [](const testing::TestParamInfo<VolumeParamTest::ParamType>& info) {
            auto descriptor = std::get<PARAM_INSTANCE_NAME>(info.param).second;
            std::string level = std::to_string(std::get<PARAM_LEVEL>(info.param));
            std::string mute = std::to_string(std::get<PARAM_MUTE>(info.param));
            std::string name = getPrefix(descriptor) + "_level" + level + "_mute" + mute;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(VolumeParamTest);

INSTANTIATE_TEST_SUITE_P(VolumeTest, VolumeDataTest,
                         testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                 IFactory::descriptor, getEffectTypeUuidVolume())),
                         [](const testing::TestParamInfo<VolumeDataTest::ParamType>& info) {
                             auto descriptor = info.param;
                             std::string name = getPrefix(descriptor.second);
                             std::replace_if(
                                     name.begin(), name.end(),
                                     [](const char c) { return !std::isalnum(c); }, '_');
                             return name;
                         });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(VolumeDataTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::UnitTest::GetInstance()->listeners().Append(new TestExecutionTracer());
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
