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

#include <map>
#include <utility>
#include <vector>

#define LOG_TAG "VtsHalHapticGeneratorTargetTest"
#include <android-base/logging.h>
#include <android/binder_enums.h>
#include <audio_utils/power.h>

#include "EffectHelper.h"

using namespace android;

using aidl::android::hardware::audio::common::getChannelCount;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::getEffectTypeUuidHapticGenerator;
using aidl::android::hardware::audio::effect::HapticGenerator;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;
using android::hardware::audio::common::testing::detail::TestExecutionTracer;

const int MIN_ID = std::numeric_limits<int>::min();
const int MAX_ID = std::numeric_limits<int>::max();
const float MIN_FLOAT = std::numeric_limits<float>::min();
const float MAX_FLOAT = std::numeric_limits<float>::max();

std::vector<HapticGenerator::VibratorScale> kScaleValues = {
        ndk::enum_range<HapticGenerator::VibratorScale>().begin(),
        ndk::enum_range<HapticGenerator::VibratorScale>().end()};

const std::vector<float> kScaleFactorValues = {HapticGenerator::HapticScale::UNDEFINED_SCALE_FACTOR,
                                               0.0f, 0.5f, 1.0f, MAX_FLOAT};
const std::vector<float> kAdaptiveScaleFactorValues = {
        HapticGenerator::HapticScale::UNDEFINED_SCALE_FACTOR, 0.0f, 0.5f, 1.0f, MAX_FLOAT};

const std::vector<float> kResonantFrequencyValues = {MIN_FLOAT, 100, MAX_FLOAT};
const std::vector<float> kQFactorValues = {MIN_FLOAT, 100, MAX_FLOAT};
const std::vector<float> kMaxAmplitude = {MIN_FLOAT, 100, MAX_FLOAT};

constexpr int HAPTIC_SCALE_FACTORS_EFFECT_MIN_VERSION = 3;

static const std::vector<int32_t> kHapticOutputLayouts = {
        AudioChannelLayout::LAYOUT_MONO_HAPTIC_A, AudioChannelLayout::LAYOUT_MONO_HAPTIC_AB,
        AudioChannelLayout::LAYOUT_STEREO_HAPTIC_A, AudioChannelLayout::LAYOUT_STEREO_HAPTIC_AB};

class HapticGeneratorHelper : public EffectHelper {
  public:
    void SetUpHapticGenerator(int32_t chMask = AudioChannelLayout::CHANNEL_HAPTIC_A) {
        ASSERT_NE(nullptr, mFactory);
        ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
        EXPECT_STATUS(EX_NONE, mEffect->getInterfaceVersion(&mEffectInterfaceVersion));

        AudioChannelLayout layout =
                AudioChannelLayout::make<AudioChannelLayout::layoutMask>(chMask);

        Parameter::Common common = createParamCommon(
                0 /* session */, 1 /* ioHandle */, kSamplingFrequency /* iSampleRate */,
                kSamplingFrequency /* oSampleRate */, kFrameCount /* iFrameCount */,
                kFrameCount /* oFrameCount */, layout, layout);
        ASSERT_NO_FATAL_FAILURE(open(mEffect, common, std::nullopt, &mOpenEffectReturn, EX_NONE));
        ASSERT_NE(nullptr, mEffect);
    }

    void TearDownHapticGenerator() {
        ASSERT_NO_FATAL_FAILURE(close(mEffect));
        ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
        mOpenEffectReturn = IEffect::OpenEffectReturn{};
    }

    Parameter createScaleParam(const std::vector<HapticGenerator::HapticScale>& hapticScales) {
        return Parameter::make<Parameter::specific>(
                Parameter::Specific::make<Parameter::Specific::hapticGenerator>(
                        HapticGenerator::make<HapticGenerator::hapticScales>(hapticScales)));
    }

    Parameter createVibratorParam(HapticGenerator::VibratorInformation vibrationInfo) {
        return Parameter::make<Parameter::specific>(
                Parameter::Specific::make<Parameter::Specific::hapticGenerator>(
                        HapticGenerator::make<HapticGenerator::vibratorInfo>(vibrationInfo)));
    }

    void setAndVerifyParameter(Parameter hapticParameter, HapticGenerator::Tag tag,
                               binder_exception_t expected = EX_NONE) {
        EXPECT_STATUS(expected, mEffect->setParameter(hapticParameter))
                << hapticParameter.toString();
        if (expected == EX_NONE) {
            // get parameter
            Parameter getParam;
            auto second = Parameter::Id::make<Parameter::Id::hapticGeneratorTag>(
                    HapticGenerator::Id::make<HapticGenerator::Id::commonTag>(
                            HapticGenerator::Tag(tag)));
            // If the set is successful, get param should match
            EXPECT_STATUS(expected, mEffect->getParameter(second, &getParam));
            EXPECT_EQ(hapticParameter, getParam) << "\nexpectedParam:" << hapticParameter.toString()
                                                 << "\ngetParam:" << getParam.toString();
        }
    }

    HapticGenerator::VibratorInformation createVibratorInfo(float resonantFrequency, float qFactor,
                                                            float amplitude) {
        return HapticGenerator::VibratorInformation(resonantFrequency, qFactor, amplitude);
    }

    static const long kFrameCount = 10000;
    static constexpr int kSamplingFrequency = 44100;
    static constexpr int kDefaultScaleID = 0;
    static constexpr float kDefaultMaxAmp = 1;
    static constexpr float kDefaultResonantFrequency = 150;
    static constexpr float kDefaultQfactor = 8;
    static constexpr HapticGenerator::VibratorScale kDefaultScale =
            HapticGenerator::VibratorScale::NONE;
    std::shared_ptr<IFactory> mFactory;
    std::shared_ptr<IEffect> mEffect;
    IEffect::OpenEffectReturn mOpenEffectReturn;
    Parameter mHapticSpecificParameter;
    Parameter::Id mHapticIdParameter;
    int mEffectInterfaceVersion;
};

/**
 *Tests do the following:
 * -Testing parameter range supported by the effect.
 * -For any supported value test expects EX_NONE from IEffect.setParameter(),
 *  otherwise expect EX_ILLEGAL_ARGUMENT.
 * -Validating the effect by comparing the output energies of the supported parameters.
 **/

using EffectInstance = std::pair<std::shared_ptr<IFactory>, Descriptor>;

class HapticGeneratorScaleParamTest : public ::testing::TestWithParam<EffectInstance>,
                                      public HapticGeneratorHelper {
  public:
    HapticGeneratorScaleParamTest() { std::tie(mFactory, mDescriptor) = GetParam(); }
    void SetUp() override { ASSERT_NO_FATAL_FAILURE(SetUpHapticGenerator()); }
    void TearDown() override { ASSERT_NO_FATAL_FAILURE(TearDownHapticGenerator()); }
};

TEST_P(HapticGeneratorScaleParamTest, SetAndGetScales) {
    std::vector<HapticGenerator::HapticScale> hapticScales;
    for (int i = 0; i < static_cast<int>(kScaleValues.size()); i++) {
        hapticScales.push_back({.id = i, .scale = kScaleValues[i]});
    }
    ASSERT_NO_FATAL_FAILURE(
            setAndVerifyParameter(createScaleParam(hapticScales), HapticGenerator::hapticScales));
}

TEST_P(HapticGeneratorScaleParamTest, SetAndGetScaleFactors) {
    if (mEffectInterfaceVersion < HAPTIC_SCALE_FACTORS_EFFECT_MIN_VERSION) {
        GTEST_SKIP() << "Skipping HapticGenerator ScaleFactors test for effect version "
                     << std::to_string(mEffectInterfaceVersion);
    }

    std::vector<HapticGenerator::HapticScale> hapticScales;
    for (int i = 0; i < static_cast<int>(kScaleFactorValues.size()); i++) {
        hapticScales.push_back(
                {.id = i, .scale = kScaleValues[0], .scaleFactor = kScaleFactorValues[i]});
    }
    ASSERT_NO_FATAL_FAILURE(
            setAndVerifyParameter(createScaleParam(hapticScales), HapticGenerator::hapticScales));
}

TEST_P(HapticGeneratorScaleParamTest, SetAndGetAdaptiveScaleFactors) {
    if (mEffectInterfaceVersion < HAPTIC_SCALE_FACTORS_EFFECT_MIN_VERSION) {
        GTEST_SKIP() << "Skipping HapticGenerator AdaptiveScaleFactors test for effect version "
                     << std::to_string(mEffectInterfaceVersion);
    }

    std::vector<HapticGenerator::HapticScale> hapticScales;
    for (int i = 0; i < static_cast<int>(kAdaptiveScaleFactorValues.size()); i++) {
        hapticScales.push_back({.id = i,
                                .scale = kScaleValues[0],
                                .scaleFactor = kScaleFactorValues[3],
                                .adaptiveScaleFactor = kAdaptiveScaleFactorValues[i]});
    }
    ASSERT_NO_FATAL_FAILURE(
            setAndVerifyParameter(createScaleParam(hapticScales), HapticGenerator::hapticScales));
}

INSTANTIATE_TEST_SUITE_P(
        HapticGeneratorValidTest, HapticGeneratorScaleParamTest,
        testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                IFactory::descriptor, getEffectTypeUuidHapticGenerator())),
        [](const testing::TestParamInfo<HapticGeneratorScaleParamTest::ParamType>& info) {
            auto descriptor = info.param;
            return getPrefix(descriptor.second);
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(HapticGeneratorScaleParamTest);

enum VibratorParamName {
    VIBRATOR_PARAM_INSTANCE,
    VIBRATOR_PARAM_RESONANT_FREQUENCY,
    VIBRATOR_PARAM_Q_FACTOR,
    VIBRATOR_PARAM_MAX_AMPLITUDE,
};

using HapticGeneratorVibratorInfoTestParam = std::tuple<EffectInstance, float, float, float>;

class HapticGeneratorVibratorInfoParamTest
    : public ::testing::TestWithParam<HapticGeneratorVibratorInfoTestParam>,
      public HapticGeneratorHelper {
  public:
    HapticGeneratorVibratorInfoParamTest()
        : mParamResonantFrequency(std::get<VIBRATOR_PARAM_RESONANT_FREQUENCY>(GetParam())),
          mParamQFactor(std::get<VIBRATOR_PARAM_Q_FACTOR>(GetParam())),
          mParamMaxAmplitude(std::get<VIBRATOR_PARAM_MAX_AMPLITUDE>(GetParam())) {
        std::tie(mFactory, mDescriptor) = std::get<VIBRATOR_PARAM_INSTANCE>(GetParam());
    }
    void SetUp() override { ASSERT_NO_FATAL_FAILURE(SetUpHapticGenerator()); }
    void TearDown() override { ASSERT_NO_FATAL_FAILURE(TearDownHapticGenerator()); }

    float mParamResonantFrequency = kDefaultResonantFrequency;
    float mParamQFactor = kDefaultQfactor;
    float mParamMaxAmplitude = kDefaultMaxAmp;
};

TEST_P(HapticGeneratorVibratorInfoParamTest, SetAndGetVibratorInformation) {
    auto vibratorInfo =
            createVibratorInfo(mParamResonantFrequency, mParamQFactor, mParamMaxAmplitude);
    if (isParameterValid<HapticGenerator, Range::hapticGenerator>(vibratorInfo, mDescriptor)) {
        ASSERT_NO_FATAL_FAILURE(setAndVerifyParameter(createVibratorParam(vibratorInfo),
                                                      HapticGenerator::vibratorInfo));
    } else {
        ASSERT_NO_FATAL_FAILURE(setAndVerifyParameter(createVibratorParam(vibratorInfo),
                                                      HapticGenerator::vibratorInfo,
                                                      EX_ILLEGAL_ARGUMENT));
    }
}
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(HapticGeneratorVibratorInfoParamTest);

INSTANTIATE_TEST_SUITE_P(
        HapticGeneratorValidTest, HapticGeneratorVibratorInfoParamTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidHapticGenerator())),
                           testing::ValuesIn(kResonantFrequencyValues),
                           testing::ValuesIn(kQFactorValues), testing::ValuesIn(kMaxAmplitude)),
        [](const testing::TestParamInfo<HapticGeneratorVibratorInfoParamTest::ParamType>& info) {
            auto descriptor = std::get<VIBRATOR_PARAM_INSTANCE>(info.param).second;
            std::string resonantFrequency =
                    std::to_string(std::get<VIBRATOR_PARAM_RESONANT_FREQUENCY>(info.param));
            std::string qFactor = std::to_string(std::get<VIBRATOR_PARAM_Q_FACTOR>(info.param));
            std::string maxAmplitude =
                    std::to_string(std::get<VIBRATOR_PARAM_MAX_AMPLITUDE>(info.param));
            std::string name = getPrefix(descriptor) + "_resonantFrequency" + resonantFrequency +
                               "_qFactor" + qFactor + "_maxAmplitude" + maxAmplitude;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

/**
 * The data tests do the following
 * -Generate test input.
 * -Check if the parameters are supported. Skip the unsupported parameter values.
 * -Validate increase in haptic output energy energy.
 **/

enum DataTestParam { EFFECT_INSTANCE, LAYOUT };
using HapticGeneratorDataTestParam = std::tuple<EffectInstance, int32_t>;

// minimal HAL interface version to run the data path test
constexpr int32_t kMinDataTestHalVersion = 3;

class HapticGeneratorDataTest : public ::testing::TestWithParam<HapticGeneratorDataTestParam>,
                                public HapticGeneratorHelper {
  public:
    HapticGeneratorDataTest() : mChMask(std::get<LAYOUT>(GetParam())) {
        std::tie(mFactory, mDescriptor) = std::get<EFFECT_INSTANCE>(GetParam());
        mAudioChannelCount =
                getChannelCount(AudioChannelLayout::make<AudioChannelLayout::layoutMask>(mChMask),
                                ~AudioChannelLayout::LAYOUT_HAPTIC_AB);
        mHapticChannelCount =
                getChannelCount(AudioChannelLayout::make<AudioChannelLayout::layoutMask>(mChMask),
                                AudioChannelLayout::LAYOUT_HAPTIC_AB);

        mAudioSamples = kFrameCount * mAudioChannelCount;
        mHapticSamples = kFrameCount * mHapticChannelCount;
        mInput.resize(mHapticSamples + mAudioSamples, 0);
        mOutput.resize(mHapticSamples + mAudioSamples, 0);
    }

    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(SetUpHapticGenerator(mChMask));
        SKIP_TEST_IF_VERSION_UNSUPPORTED(mEffect, kMinDataTestHalVersion);
    }

    void TearDown() override { ASSERT_NO_FATAL_FAILURE(TearDownHapticGenerator()); }

    void generateSinePeriod() {
        size_t cycleSize = kSamplingFrequency / kInputFrequency;
        size_t startSize = 0;
        while (startSize < mAudioSamples) {
            for (size_t i = 0; i < cycleSize; i++) {
                mInput[i + startSize] = sin(2 * M_PI * kInputFrequency * i / kSamplingFrequency);
            }
            startSize += mAudioSamples / 4;
        }
    }

    void setBaseVibratorParam() {
        auto vibratorInfo =
                createVibratorInfo(kDefaultResonantFrequency, kDefaultQfactor, kDefaultMaxAmp);
        if (isParameterValid<HapticGenerator, Range::hapticGenerator>(vibratorInfo, mDescriptor)) {
            ASSERT_NO_FATAL_FAILURE(setAndVerifyParameter(createVibratorParam(vibratorInfo),
                                                          HapticGenerator::vibratorInfo));
        } else {
            GTEST_SKIP() << "Invalid base vibrator values, skipping the test\n";
        }
    }

    void setBaseScaleParam() {
        ASSERT_NO_FATAL_FAILURE(setAndVerifyParameter(
                createScaleParam({HapticGenerator::HapticScale(kDefaultScaleID, kDefaultScale)}),
                HapticGenerator::hapticScales));
    }

    void validateIncreasingEnergy(HapticGenerator::Tag tag) {
        float baseEnergy = -1;
        for (auto param : mHapticParam) {
            ASSERT_NO_FATAL_FAILURE(setAndVerifyParameter(param, tag));
            SCOPED_TRACE("Param: " + param.toString());
            ASSERT_NO_FATAL_FAILURE(
                    processAndWriteToOutput(mInput, mOutput, mEffect, mOpenEffectReturn));
            float hapticOutputEnergy = audio_utils_compute_energy_mono(
                    mOutput.data() + mAudioSamples, AUDIO_FORMAT_PCM_FLOAT, mHapticSamples);
            EXPECT_GT(hapticOutputEnergy, baseEnergy);
            baseEnergy = hapticOutputEnergy;
        }
    }

    float findAbsMax(auto begin, auto end) {
        return *std::max_element(begin, end,
                                 [](float a, float b) { return std::abs(a) < std::abs(b); });
    }

    void findMaxAmplitude() {
        for (float amp = 0.1; amp <= 1; amp += 0.1) {
            auto vibratorInfo = createVibratorInfo(kDefaultResonantFrequency, kDefaultQfactor, amp);
            if (!isParameterValid<HapticGenerator, Range::hapticGenerator>(vibratorInfo,
                                                                           mDescriptor)) {
                continue;
            }
            ASSERT_NO_FATAL_FAILURE(setAndVerifyParameter(createVibratorParam(vibratorInfo),
                                                          HapticGenerator::vibratorInfo));
            ASSERT_NO_FATAL_FAILURE(
                    processAndWriteToOutput(mInput, mOutput, mEffect, mOpenEffectReturn));
            float outAmplitude = findAbsMax(mOutput.begin() + mAudioSamples, mOutput.end());
            if (outAmplitude > mMaxAmplitude) {
                mMaxAmplitude = outAmplitude;
            } else {
                break;
            }
        }
    }

    const int kInputFrequency = 1000;
    float mMaxAmplitude = 0;
    size_t mHapticSamples;
    int32_t mChMask;
    int32_t mAudioChannelCount;
    int32_t mHapticChannelCount;
    size_t mAudioSamples;
    float mBaseHapticOutputEnergy;
    std::vector<Parameter> mHapticParam;
    // both input and output buffer includes audio and haptic samples
    std::vector<float> mInput;
    std::vector<float> mOutput;
};

TEST_P(HapticGeneratorDataTest, IncreasingVibratorScaleTest) {
    generateInput(mInput, kInputFrequency, kSamplingFrequency, mAudioSamples);
    ASSERT_NO_FATAL_FAILURE(setBaseVibratorParam());
    for (HapticGenerator::VibratorScale scale : kScaleValues) {
        mHapticParam.push_back(
                createScaleParam({HapticGenerator::HapticScale(kDefaultScaleID, scale)}));
    }
    ASSERT_NO_FATAL_FAILURE(validateIncreasingEnergy(HapticGenerator::hapticScales));
}

TEST_P(HapticGeneratorDataTest, IncreasingMaxAmplitudeTest) {
    generateInput(mInput, kInputFrequency, kSamplingFrequency, mAudioSamples);
    ASSERT_NO_FATAL_FAILURE(setBaseScaleParam());
    findMaxAmplitude();
    std::vector<float> increasingAmplitudeValues = {0.25f * mMaxAmplitude, 0.5f * mMaxAmplitude,
                                                    0.75f * mMaxAmplitude, mMaxAmplitude};
    for (float amplitude : increasingAmplitudeValues) {
        auto vibratorInfo =
                createVibratorInfo(kDefaultResonantFrequency, kDefaultQfactor, amplitude);
        if (!isParameterValid<HapticGenerator, Range::hapticGenerator>(vibratorInfo, mDescriptor)) {
            continue;
        }
        mHapticParam.push_back(createVibratorParam(vibratorInfo));
    }
    ASSERT_NO_FATAL_FAILURE(validateIncreasingEnergy(HapticGenerator::vibratorInfo));
}

TEST_P(HapticGeneratorDataTest, DescreasingResonantFrequencyTest) {
    std::vector<float> descreasingResonantFrequency = {800, 600, 400, 200};
    generateInput(mInput, kInputFrequency, kSamplingFrequency, mAudioSamples);
    ASSERT_NO_FATAL_FAILURE(setBaseScaleParam());
    for (float resonantFrequency : descreasingResonantFrequency) {
        auto vibratorInfo = createVibratorInfo(resonantFrequency, kDefaultQfactor, kDefaultMaxAmp);
        if (!isParameterValid<HapticGenerator, Range::hapticGenerator>(vibratorInfo, mDescriptor)) {
            continue;
        }
        mHapticParam.push_back(createVibratorParam(vibratorInfo));
    }
    ASSERT_NO_FATAL_FAILURE(validateIncreasingEnergy(HapticGenerator::vibratorInfo));
}

TEST_P(HapticGeneratorDataTest, IncreasingQfactorTest) {
    std::vector<float> increasingQfactor = {16, 24, 32, 40};
    generateSinePeriod();
    ASSERT_NO_FATAL_FAILURE(setBaseScaleParam());
    for (float qFactor : increasingQfactor) {
        auto vibratorInfo = createVibratorInfo(kDefaultResonantFrequency, qFactor, kDefaultMaxAmp);
        if (!isParameterValid<HapticGenerator, Range::hapticGenerator>(vibratorInfo, mDescriptor)) {
            continue;
        }
        mHapticParam.push_back(createVibratorParam(vibratorInfo));
    }
    ASSERT_NO_FATAL_FAILURE(validateIncreasingEnergy(HapticGenerator::vibratorInfo));
}

INSTANTIATE_TEST_SUITE_P(
        DataTest, HapticGeneratorDataTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidHapticGenerator())),
                           testing::ValuesIn(kHapticOutputLayouts)),
        [](const testing::TestParamInfo<HapticGeneratorDataTest::ParamType>& info) {
            auto descriptor = std::get<EFFECT_INSTANCE>(info.param).second;
            std::string layout = "0x" + std::format("{:x}", std::get<LAYOUT>(info.param));
            std::string name = getPrefix(descriptor) + "_layout_" + layout;
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(HapticGeneratorDataTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::UnitTest::GetInstance()->listeners().Append(new TestExecutionTracer());
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
