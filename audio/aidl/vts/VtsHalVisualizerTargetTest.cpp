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

#include <unordered_set>

#define LOG_TAG "VtsHalVisualizerTest"
#include <android-base/logging.h>
#include <android/binder_enums.h>
#include <audio_utils/power.h>

#include "EffectHelper.h"

using namespace android;

using aidl::android::hardware::audio::common::getChannelCount;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::getEffectTypeUuidVisualizer;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;
using aidl::android::hardware::audio::effect::Visualizer;
using android::hardware::audio::common::testing::detail::TestExecutionTracer;

/**
 * Here we focus on specific parameter checking, general IEffect interfaces testing performed in
 * VtsAudioEffectTargetTest.
 */
enum ParamName {
    PARAM_INSTANCE_NAME,
    PARAM_CAPTURE_SIZE,
    PARAM_SCALING_MODE,
    PARAM_MEASUREMENT_MODE,
    PARAM_LATENCY,
};
using VisualizerTestParam = std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int,
                                       Visualizer::ScalingMode, Visualizer::MeasurementMode, int>;

class VisualizerTestHelper : public EffectHelper {
  public:
    VisualizerTestHelper(
            std::pair<std::shared_ptr<IFactory>, Descriptor> descPair = {}, int captureSize = 128,
            int latency = 0,
            Visualizer::ScalingMode scalingMode = Visualizer::ScalingMode::NORMALIZED,
            Visualizer::MeasurementMode measurementMode = Visualizer::MeasurementMode::NONE)
        : mCaptureSize(captureSize),
          mLatency(latency),
          mScalingMode(scalingMode),
          mMeasurementMode(measurementMode),
          mInputBuffer(mBufferSizeInFrames),
          mOutputBuffer(mBufferSizeInFrames) {
        std::tie(mFactory, mDescriptor) = descPair;
    }

    void SetUpVisualizer() {
        ASSERT_NE(nullptr, mFactory);
        ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));

        Parameter::Common common = createParamCommon(
                0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */,
                kInputFrameCount /* iFrameCount */, kOutputFrameCount /* oFrameCount */);
        ASSERT_NO_FATAL_FAILURE(open(mEffect, common, std::nullopt, &mOpenEffectReturn, EX_NONE));
        ASSERT_NE(nullptr, mEffect);
        mVersion = EffectFactoryHelper::getHalVersion(mFactory);
    }

    void TearDownVisualizer() {
        ASSERT_NO_FATAL_FAILURE(close(mEffect));
        ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
        mOpenEffectReturn = IEffect::OpenEffectReturn{};
    }

    void SetAndGetParameters(bool* allParamsValid = nullptr) {
        if (allParamsValid != nullptr) *allParamsValid = true;
        for (auto& it : mCommonTags) {
            auto& tag = it.first;
            auto& vs = it.second;

            // validate parameter
            Descriptor desc;
            ASSERT_STATUS(EX_NONE, mEffect->getDescriptor(&desc));
            const bool valid = isParameterValid<Visualizer, Range::visualizer>(vs, desc);
            const binder_exception_t expected = valid ? EX_NONE : EX_ILLEGAL_ARGUMENT;
            if (expected == EX_ILLEGAL_ARGUMENT && allParamsValid != nullptr) {
                *allParamsValid = false;
            }

            // set parameter
            Parameter expectParam;
            Parameter::Specific specific;
            specific.set<Parameter::Specific::visualizer>(vs);
            expectParam.set<Parameter::specific>(specific);
            EXPECT_STATUS(expected, mEffect->setParameter(expectParam)) << expectParam.toString();

            // only get if parameter in range and set success
            if (expected == EX_NONE) {
                Parameter getParam;
                Parameter::Id id;
                Visualizer::Id vsId;
                vsId.set<Visualizer::Id::commonTag>(tag);
                id.set<Parameter::Id::visualizerTag>(vsId);
                EXPECT_STATUS(EX_NONE, mEffect->getParameter(id, &getParam))
                        << " with: " << id.toString();
                EXPECT_EQ(expectParam, getParam) << "\nexpect:" << expectParam.toString()
                                                 << "\ngetParam:" << getParam.toString();
            }
        }
    }

    void addCaptureSizeParam(int captureSize) {
        mCommonTags.push_back({Visualizer::captureSamples,
                               Visualizer::make<Visualizer::captureSamples>(captureSize)});
    }

    void addScalingModeParam(Visualizer::ScalingMode scalingMode) {
        mCommonTags.push_back(
                {Visualizer::scalingMode, Visualizer::make<Visualizer::scalingMode>(scalingMode)});
    }

    void addMeasurementModeParam(Visualizer::MeasurementMode measurementMode) {
        mCommonTags.push_back({Visualizer::measurementMode,
                               Visualizer::make<Visualizer::measurementMode>(measurementMode)});
    }

    void addLatencyParam(int latency) {
        mCommonTags.push_back(
                {Visualizer::latencyMs, Visualizer::make<Visualizer::latencyMs>(latency)});
    }

    static std::unordered_set<Visualizer::ScalingMode> getScalingModeValues() {
        return {ndk::enum_range<Visualizer::ScalingMode>().begin(),
                ndk::enum_range<Visualizer::ScalingMode>().end()};
    }

    static constexpr long kInputFrameCount = 0x100, kOutputFrameCount = 0x100;
    const size_t mChannelCount =
            getChannelCount(AudioChannelLayout::make<AudioChannelLayout::layoutMask>(
                    AudioChannelLayout::LAYOUT_MONO));
    const size_t mBufferSizeInFrames = kInputFrameCount * mChannelCount;
    const int mCaptureSize;
    const int mLatency;
    const Visualizer::ScalingMode mScalingMode;
    const Visualizer::MeasurementMode mMeasurementMode;
    int mVersion;
    std::vector<float> mInputBuffer;
    std::vector<float> mOutputBuffer;
    std::shared_ptr<IEffect> mEffect;
    std::shared_ptr<IFactory> mFactory;
    Descriptor mDescriptor;
    IEffect::OpenEffectReturn mOpenEffectReturn;

  private:
    std::vector<std::pair<Visualizer::Tag, Visualizer>> mCommonTags;
    void CleanUp() { mCommonTags.clear(); }
};

class VisualizerParamTest : public ::testing::TestWithParam<VisualizerTestParam>,
                            public VisualizerTestHelper {
  public:
    VisualizerParamTest()
        : VisualizerTestHelper(std::get<PARAM_INSTANCE_NAME>(GetParam()),
                               std::get<PARAM_CAPTURE_SIZE>(GetParam()),
                               std::get<PARAM_LATENCY>(GetParam()),
                               std::get<PARAM_SCALING_MODE>(GetParam()),
                               std::get<PARAM_MEASUREMENT_MODE>(GetParam())) {
        generateInputBuffer(mInputBuffer, 0, true, mChannelCount, kMaxAudioSampleValue);
    }

    void SetUp() override { SetUpVisualizer(); }

    void TearDown() override { TearDownVisualizer(); }

    static std::unordered_set<Visualizer::MeasurementMode> getMeasurementModeValues() {
        return {ndk::enum_range<Visualizer::MeasurementMode>().begin(),
                ndk::enum_range<Visualizer::MeasurementMode>().end()};
    }
};

TEST_P(VisualizerParamTest, SetAndGetCaptureSize) {
    ASSERT_NO_FATAL_FAILURE(addCaptureSizeParam(mCaptureSize));
    ASSERT_NO_FATAL_FAILURE(SetAndGetParameters());
}

TEST_P(VisualizerParamTest, SetAndGetScalingMode) {
    ASSERT_NO_FATAL_FAILURE(addScalingModeParam(mScalingMode));
    ASSERT_NO_FATAL_FAILURE(SetAndGetParameters());
}

TEST_P(VisualizerParamTest, SetAndGetMeasurementMode) {
    ASSERT_NO_FATAL_FAILURE(addMeasurementModeParam(mMeasurementMode));
    ASSERT_NO_FATAL_FAILURE(SetAndGetParameters());
}

TEST_P(VisualizerParamTest, SetAndGetLatency) {
    ASSERT_NO_FATAL_FAILURE(addLatencyParam(mLatency));
    ASSERT_NO_FATAL_FAILURE(SetAndGetParameters());
}

TEST_P(VisualizerParamTest, testCaptureSampleBufferSizeAndOutput) {
    SKIP_TEST_IF_DATA_UNSUPPORTED(mDescriptor.common.flags);

    bool allParamsValid = true;
    ASSERT_NO_FATAL_FAILURE(addCaptureSizeParam(mCaptureSize));
    ASSERT_NO_FATAL_FAILURE(addScalingModeParam(mScalingMode));
    ASSERT_NO_FATAL_FAILURE(addMeasurementModeParam(mMeasurementMode));
    ASSERT_NO_FATAL_FAILURE(addLatencyParam(mLatency));
    ASSERT_NO_FATAL_FAILURE(SetAndGetParameters(&allParamsValid));

    Parameter getParam;
    Parameter::Id id;
    Visualizer::Id vsId;
    vsId.set<Visualizer::Id::commonTag>(Visualizer::captureSampleBuffer);
    id.set<Parameter::Id::visualizerTag>(vsId);
    EXPECT_STATUS(EX_NONE, mEffect->getParameter(id, &getParam)) << " with: " << id.toString();

    ASSERT_NO_FATAL_FAILURE(processAndWriteToOutput(mInputBuffer, mOutputBuffer, mEffect,
                                                    &mOpenEffectReturn, mVersion));
    ASSERT_EQ(mInputBuffer, mOutputBuffer);

    if (allParamsValid) {
        std::vector<uint8_t> captureBuffer = getParam.get<Parameter::specific>()
                                                     .get<Parameter::Specific::visualizer>()
                                                     .get<Visualizer::captureSampleBuffer>();
        ASSERT_EQ((size_t)mCaptureSize, captureBuffer.size());
    }
}

class VisualizerDataTest : public ::testing::TestWithParam<VisualizerTestParam>,
                           public VisualizerTestHelper {
  public:
    VisualizerDataTest()
        : VisualizerTestHelper(std::get<PARAM_INSTANCE_NAME>(GetParam()),
                               std::get<PARAM_CAPTURE_SIZE>(GetParam()),
                               std::get<PARAM_LATENCY>(GetParam()),
                               std::get<PARAM_SCALING_MODE>(GetParam()),
                               std::get<PARAM_MEASUREMENT_MODE>(GetParam())) {}

    void SetUp() override { SetUpVisualizer(); }

    void TearDown() override { TearDownVisualizer(); }
};

TEST_P(VisualizerDataTest, testScalingModeParameters) {
    SKIP_TEST_IF_DATA_UNSUPPORTED(mDescriptor.common.flags);

    // This test holds true for the following range
    static_assert(kMaxAudioSampleValue <= 1.0 && kMaxAudioSampleValue > 0.0,
                  "Valid range of kMaxAudioSample value for the test: (0.0, 1.0]");

    constexpr float kPowerToleranceDb = 0.5;

    generateSineWave(std::vector<int>{1000}, mInputBuffer, 1.0, mBufferSizeInFrames);
    const float expectedPowerNormalized = audio_utils_compute_power_mono(
            mInputBuffer.data(), AUDIO_FORMAT_PCM_FLOAT, mInputBuffer.size());

    const std::vector<float> testMaxAudioSampleValueList = {
            0.25 * kMaxAudioSampleValue, 0.5 * kMaxAudioSampleValue, 0.75 * kMaxAudioSampleValue,
            kMaxAudioSampleValue};

    Parameter::Id idCsb;
    Visualizer::Id vsIdCsb;
    vsIdCsb.set<Visualizer::Id::commonTag>(Visualizer::captureSampleBuffer);
    idCsb.set<Parameter::Id::visualizerTag>(vsIdCsb);

    for (float maxAudioSampleValue : testMaxAudioSampleValueList) {
        bool allParamsValid = true;
        ASSERT_NO_FATAL_FAILURE(addCaptureSizeParam(mCaptureSize));
        ASSERT_NO_FATAL_FAILURE(addScalingModeParam(mScalingMode));
        ASSERT_NO_FATAL_FAILURE(addLatencyParam(mLatency));
        ASSERT_NO_FATAL_FAILURE(SetAndGetParameters(&allParamsValid));

        generateSineWave(std::vector<int>{1000}, mInputBuffer, maxAudioSampleValue,
                         mBufferSizeInFrames);

        // The stop and reset calls to the effect are made towards the end in order to fetch the
        // captureSampleBuffer values
        ASSERT_NO_FATAL_FAILURE(processAndWriteToOutput(mInputBuffer, mOutputBuffer, mEffect,
                                                        &mOpenEffectReturn, mVersion, 1, false));
        if (allParamsValid) {
            Parameter getParam;
            EXPECT_STATUS(EX_NONE, mEffect->getParameter(idCsb, &getParam))
                    << " with: " << idCsb.toString();
            std::vector<uint8_t> captureBuffer = getParam.get<Parameter::specific>()
                                                         .get<Parameter::Specific::visualizer>()
                                                         .get<Visualizer::captureSampleBuffer>();
            ASSERT_EQ((size_t)mCaptureSize, captureBuffer.size());

            float currPowerCsb = audio_utils_compute_power_mono(
                    captureBuffer.data(), AUDIO_FORMAT_PCM_8_BIT, mCaptureSize);

            if (mScalingMode == Visualizer::ScalingMode::NORMALIZED) {
                EXPECT_NEAR(currPowerCsb, expectedPowerNormalized, kPowerToleranceDb);
            } else {
                float powerI = audio_utils_compute_power_mono(
                        mInputBuffer.data(), AUDIO_FORMAT_PCM_FLOAT, mInputBuffer.size());
                EXPECT_NEAR(currPowerCsb, powerI, kPowerToleranceDb);
            }
        }
        ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::STOP));
        ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::RESET));
    }
}

std::vector<std::pair<std::shared_ptr<IFactory>, Descriptor>> kDescPair;
INSTANTIATE_TEST_SUITE_P(
        VisualizerParamTest, VisualizerParamTest,
        ::testing::Combine(
                testing::ValuesIn(kDescPair = EffectFactoryHelper::getAllEffectDescriptors(
                                          IFactory::descriptor, getEffectTypeUuidVisualizer())),
                testing::ValuesIn(EffectHelper::getTestValueSet<Visualizer, int, Range::visualizer,
                                                                Visualizer::captureSamples>(
                        kDescPair, EffectHelper::expandTestValueBasic<int>)),
                testing::ValuesIn(VisualizerTestHelper::getScalingModeValues()),
                testing::ValuesIn(VisualizerParamTest::getMeasurementModeValues()),
                testing::ValuesIn(EffectHelper::getTestValueSet<Visualizer, int, Range::visualizer,
                                                                Visualizer::latencyMs>(
                        kDescPair, EffectHelper::expandTestValueBasic<int>))),
        [](const testing::TestParamInfo<VisualizerParamTest::ParamType>& info) {
            auto descriptor = std::get<PARAM_INSTANCE_NAME>(info.param).second;
            std::string captureSize = std::to_string(std::get<PARAM_CAPTURE_SIZE>(info.param));
            std::string scalingMode = aidl::android::hardware::audio::effect::toString(
                    std::get<PARAM_SCALING_MODE>(info.param));
            std::string measurementMode = aidl::android::hardware::audio::effect::toString(
                    std::get<PARAM_MEASUREMENT_MODE>(info.param));
            std::string latency = std::to_string(std::get<PARAM_LATENCY>(info.param));

            std::string name = getPrefix(descriptor) + "_captureSize" + captureSize +
                               "_scalingMode" + scalingMode + "_measurementMode" + measurementMode +
                               "_latency" + latency;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(VisualizerParamTest);

INSTANTIATE_TEST_SUITE_P(
        VisualizerDataTest, VisualizerDataTest,
        ::testing::Combine(
                testing::ValuesIn(kDescPair = EffectFactoryHelper::getAllEffectDescriptors(
                                          IFactory::descriptor, getEffectTypeUuidVisualizer())),
                testing::Values(128),  // captureSize
                testing::ValuesIn(VisualizerTestHelper::getScalingModeValues()),
                testing::Values(Visualizer::MeasurementMode::PEAK_RMS),
                testing::Values(0)  // latency
                ),
        [](const testing::TestParamInfo<VisualizerDataTest::ParamType>& info) {
            auto descriptor = std::get<PARAM_INSTANCE_NAME>(info.param).second;
            std::string captureSize = std::to_string(std::get<PARAM_CAPTURE_SIZE>(info.param));
            std::string scalingMode = aidl::android::hardware::audio::effect::toString(
                    std::get<PARAM_SCALING_MODE>(info.param));
            std::string measurementMode = aidl::android::hardware::audio::effect::toString(
                    std::get<PARAM_MEASUREMENT_MODE>(info.param));
            std::string latency = std::to_string(std::get<PARAM_LATENCY>(info.param));

            std::string name = getPrefix(descriptor) + "_captureSize" + captureSize +
                               "_scalingMode" + scalingMode + "_measurementMode" + measurementMode +
                               "_latency" + latency;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(VisualizerDataTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::UnitTest::GetInstance()->listeners().Append(new TestExecutionTracer());
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
