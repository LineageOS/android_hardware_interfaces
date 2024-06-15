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

#include <limits.h>

#define LOG_TAG "VtsHalBassBoostTest"
#include <aidl/Vintf.h>
#include <android-base/logging.h>
#include "EffectHelper.h"
#include "pffft.hpp"

using namespace android;

using aidl::android::hardware::audio::common::getChannelCount;
using aidl::android::hardware::audio::effect::BassBoost;
using aidl::android::hardware::audio::effect::Capability;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::getEffectTypeUuidBassBoost;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;
using aidl::android::hardware::audio::effect::Range;
using android::hardware::audio::common::testing::detail::TestExecutionTracer;

// minimal HAL interface version to run bassboost data path test
constexpr int32_t kMinDataTestHalVersion = 2;
static const std::vector<int32_t> kLayouts = {AudioChannelLayout::LAYOUT_STEREO,
                                              AudioChannelLayout::LAYOUT_MONO};
/*
 * Testing parameter range, assuming the parameter supported by effect is in this range.
 * Parameter should be within the valid range defined in the documentation,
 * for any supported value test expects EX_NONE from IEffect.setParameter(),
 * otherwise expect EX_ILLEGAL_ARGUMENT.
 */

class BassBoostEffectHelper : public EffectHelper {
  public:
    void SetUpBassBoost(int32_t layout = AudioChannelLayout::LAYOUT_STEREO) {
        ASSERT_NE(nullptr, mFactory);
        ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
        setFrameCounts(layout);

        AudioChannelLayout channelLayout =
                AudioChannelLayout::make<AudioChannelLayout::layoutMask>(layout);

        Parameter::Specific specific = getDefaultParamSpecific();
        Parameter::Common common = EffectHelper::createParamCommon(
                0 /* session */, 1 /* ioHandle */, kSamplingFrequency /* iSampleRate */,
                kSamplingFrequency /* oSampleRate */, mInputFrameCount /* iFrameCount */,
                mOutputFrameCount /* oFrameCount */, channelLayout, channelLayout);
        ASSERT_NO_FATAL_FAILURE(open(mEffect, common, specific, &mOpenEffectReturn, EX_NONE));
        ASSERT_NE(nullptr, mEffect);
    }

    void TearDownBassBoost() {
        ASSERT_NO_FATAL_FAILURE(close(mEffect));
        ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
        mOpenEffectReturn = IEffect::OpenEffectReturn{};
    }

    Parameter::Specific getDefaultParamSpecific() {
        BassBoost bb = BassBoost::make<BassBoost::strengthPm>(0);
        Parameter::Specific specific =
                Parameter::Specific::make<Parameter::Specific::bassBoost>(bb);
        return specific;
    }

    void setFrameCounts(int32_t inputBufferLayout) {
        int channelCount = getChannelCount(
                AudioChannelLayout::make<AudioChannelLayout::layoutMask>(inputBufferLayout));
        mInputFrameCount = kInputSize / channelCount;
        mOutputFrameCount = kInputSize / channelCount;
    }

    Parameter createBassBoostParam(int strength) {
        return Parameter::make<Parameter::specific>(
                Parameter::Specific::make<Parameter::Specific::bassBoost>(
                        BassBoost::make<BassBoost::strengthPm>(strength)));
    }

    bool isStrengthValid(int strength) {
        auto bb = BassBoost::make<BassBoost::strengthPm>(strength);
        return isParameterValid<BassBoost, Range::bassBoost>(bb, mDescriptor);
    }

    void setAndVerifyParameters(int strength, binder_exception_t expected) {
        auto expectedParam = createBassBoostParam(strength);
        EXPECT_STATUS(expected, mEffect->setParameter(expectedParam)) << expectedParam.toString();

        if (expected == EX_NONE) {
            auto bbId = BassBoost::Id::make<BassBoost::Id::commonTag>(
                    BassBoost::Tag(BassBoost::strengthPm));
            auto id = Parameter::Id::make<Parameter::Id::bassBoostTag>(bbId);
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
    static constexpr int kInputSize = kSamplingFrequency * kDurationMilliSec / 1000;
    long mInputFrameCount, mOutputFrameCount;
    std::shared_ptr<IFactory> mFactory;
    Descriptor mDescriptor;
    std::shared_ptr<IEffect> mEffect;
    IEffect::OpenEffectReturn mOpenEffectReturn;
};

/**
 * Here we focus on specific parameter checking, general IEffect interfaces testing performed in
 * VtsAudioEffectTargetTest.
 */
enum ParamName { PARAM_INSTANCE_NAME, PARAM_STRENGTH };
using BassBoostParamTestParam = std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int>;

class BassBoostParamTest : public ::testing::TestWithParam<BassBoostParamTestParam>,
                           public BassBoostEffectHelper {
  public:
    BassBoostParamTest() : mParamStrength(std::get<PARAM_STRENGTH>(GetParam())) {
        std::tie(mFactory, mDescriptor) = std::get<PARAM_INSTANCE_NAME>(GetParam());
    }

    void SetUp() override { ASSERT_NO_FATAL_FAILURE(SetUpBassBoost()); }
    void TearDown() override { TearDownBassBoost(); }

    int mParamStrength = 0;
};

TEST_P(BassBoostParamTest, SetAndGetStrength) {
    if (isStrengthValid(mParamStrength)) {
        ASSERT_NO_FATAL_FAILURE(setAndVerifyParameters(mParamStrength, EX_NONE));
    } else {
        ASSERT_NO_FATAL_FAILURE(setAndVerifyParameters(mParamStrength, EX_ILLEGAL_ARGUMENT));
    }
}

enum DataParamName { DATA_INSTANCE_NAME, DATA_LAYOUT };

using BassBoostDataTestParam =
        std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int32_t>;

class BassBoostDataTest : public ::testing::TestWithParam<BassBoostDataTestParam>,
                          public BassBoostEffectHelper {
  public:
    BassBoostDataTest() : mChannelLayout(std::get<DATA_LAYOUT>(GetParam())) {
        std::tie(mFactory, mDescriptor) = std::get<DATA_INSTANCE_NAME>(GetParam());
        mStrengthValues = getTestValueSet<BassBoost, int, Range::bassBoost, BassBoost::strengthPm>(
                {std::get<DATA_INSTANCE_NAME>(GetParam())}, expandTestValueBasic<int>);
    }

    void SetUp() override {
        SKIP_TEST_IF_DATA_UNSUPPORTED(mDescriptor.common.flags);
        ASSERT_NO_FATAL_FAILURE(SetUpBassBoost(mChannelLayout));
        if (int32_t version;
            mEffect->getInterfaceVersion(&version).isOk() && version < kMinDataTestHalVersion) {
            GTEST_SKIP() << "Skipping the data test for version: " << version << "\n";
        }
    }

    void TearDown() override {
        SKIP_TEST_IF_DATA_UNSUPPORTED(mDescriptor.common.flags);
        TearDownBassBoost();
    }

    // Find FFT bin indices for testFrequencies and get bin center frequencies
    void roundToFreqCenteredToFftBin(std::vector<int>& testFrequencies,
                                     std::vector<int>& binOffsets) {
        for (size_t i = 0; i < testFrequencies.size(); i++) {
            binOffsets[i] = std::round(testFrequencies[i] / kBinWidth);
            testFrequencies[i] = std::round(binOffsets[i] * kBinWidth);
        }
    }

    // Generate multitone input between -1 to +1 using testFrequencies
    void generateMultiTone(const std::vector<int>& testFrequencies, std::vector<float>& input) {
        for (auto i = 0; i < kInputSize; i++) {
            input[i] = 0;

            for (size_t j = 0; j < testFrequencies.size(); j++) {
                input[i] += sin(2 * M_PI * testFrequencies[j] * i / kSamplingFrequency);
            }
            input[i] /= testFrequencies.size();
        }
    }

    // Use FFT transform to convert the buffer to frequency domain
    // Compute its magnitude at binOffsets
    std::vector<float> calculateMagnitude(const std::vector<float>& buffer,
                                          const std::vector<int>& binOffsets) {
        std::vector<float> fftInput(kNPointFFT);
        PFFFT_Setup* inputHandle = pffft_new_setup(kNPointFFT, PFFFT_REAL);
        pffft_transform_ordered(inputHandle, buffer.data(), fftInput.data(), nullptr,
                                PFFFT_FORWARD);
        pffft_destroy_setup(inputHandle);
        std::vector<float> bufferMag(binOffsets.size());
        for (size_t i = 0; i < binOffsets.size(); i++) {
            size_t k = binOffsets[i];
            bufferMag[i] = sqrt((fftInput[k * 2] * fftInput[k * 2]) +
                                (fftInput[k * 2 + 1] * fftInput[k * 2 + 1]));
        }

        return bufferMag;
    }

    // Calculate gain difference between low frequency and high frequency magnitude
    float calculateGainDiff(const std::vector<float>& inputMag,
                            const std::vector<float>& outputMag) {
        std::vector<float> gains(inputMag.size());

        for (size_t i = 0; i < inputMag.size(); i++) {
            gains[i] = 20 * log10(outputMag[i] / inputMag[i]);
        }

        return gains[0] - gains[1];
    }

    static constexpr int kNPointFFT = 32768;
    static constexpr float kBinWidth = (float)kSamplingFrequency / kNPointFFT;
    std::set<int> mStrengthValues;
    int32_t mChannelLayout;
};

TEST_P(BassBoostDataTest, IncreasingStrength) {
    // Frequencies to generate multitone input
    std::vector<int> testFrequencies = {100, 1000};

    // FFT bin indices for testFrequencies
    std::vector<int> binOffsets(testFrequencies.size());

    std::vector<float> input(kInputSize);
    std::vector<float> baseOutput(kInputSize);

    std::vector<float> inputMag(testFrequencies.size());
    float prevGain = -100;

    roundToFreqCenteredToFftBin(testFrequencies, binOffsets);

    generateMultiTone(testFrequencies, input);

    inputMag = calculateMagnitude(input, binOffsets);

    if (isStrengthValid(0)) {
        ASSERT_NO_FATAL_FAILURE(setAndVerifyParameters(0, EX_NONE));
    } else {
        GTEST_SKIP() << "Strength not supported, skipping the test\n";
    }

    ASSERT_NO_FATAL_FAILURE(
            processAndWriteToOutput(input, baseOutput, mEffect, &mOpenEffectReturn));

    std::vector<float> baseMag(testFrequencies.size());
    baseMag = calculateMagnitude(baseOutput, binOffsets);
    float baseDiff = calculateGainDiff(inputMag, baseMag);

    for (int strength : mStrengthValues) {
        // Skipping the further steps for invalid strength values
        if (!isStrengthValid(strength)) {
            continue;
        }

        ASSERT_NO_FATAL_FAILURE(setAndVerifyParameters(strength, EX_NONE));

        std::vector<float> output(kInputSize);
        std::vector<float> outputMag(testFrequencies.size());

        ASSERT_NO_FATAL_FAILURE(
                processAndWriteToOutput(input, output, mEffect, &mOpenEffectReturn));

        outputMag = calculateMagnitude(output, binOffsets);
        float diff = calculateGainDiff(inputMag, outputMag);

        ASSERT_GT(diff, prevGain);
        ASSERT_GT(diff, baseDiff);
        prevGain = diff;
    }
}

std::vector<std::pair<std::shared_ptr<IFactory>, Descriptor>> kDescPair;
INSTANTIATE_TEST_SUITE_P(
        BassBoostTest, BassBoostParamTest,
        ::testing::Combine(
                testing::ValuesIn(kDescPair = EffectFactoryHelper::getAllEffectDescriptors(
                                          IFactory::descriptor, getEffectTypeUuidBassBoost())),
                testing::ValuesIn(EffectHelper::getTestValueSet<BassBoost, int, Range::bassBoost,
                                                                BassBoost::strengthPm>(
                        kDescPair, EffectHelper::expandTestValueBasic<int>))),
        [](const testing::TestParamInfo<BassBoostParamTest::ParamType>& info) {
            auto descriptor = std::get<PARAM_INSTANCE_NAME>(info.param).second;
            std::string strength = std::to_string(std::get<PARAM_STRENGTH>(info.param));
            std::string name = getPrefix(descriptor) + "_strength_" + strength;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(BassBoostParamTest);

INSTANTIATE_TEST_SUITE_P(
        BassBoostTest, BassBoostDataTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidBassBoost())),
                           testing::ValuesIn(kLayouts)),
        [](const testing::TestParamInfo<BassBoostDataTest::ParamType>& info) {
            auto descriptor = std::get<DATA_INSTANCE_NAME>(info.param).second;
            std::string layout = std::to_string(std::get<DATA_LAYOUT>(info.param));
            std::string name = getPrefix(descriptor) + "_layout_" + layout;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(BassBoostDataTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::UnitTest::GetInstance()->listeners().Append(new TestExecutionTracer());
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
