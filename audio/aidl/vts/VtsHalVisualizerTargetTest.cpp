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

#define LOG_TAG "VtsHalVisualizerTest"

#include <Utils.h>
#include "EffectHelper.h"

using namespace android;

using aidl::android::hardware::audio::effect::Capability;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::kVisualizerTypeUUID;
using aidl::android::hardware::audio::effect::Parameter;
using aidl::android::hardware::audio::effect::Visualizer;

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
using VisualizerParamTestParam =
        std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int, Visualizer::ScalingMode,
                   Visualizer::MeasurementMode, int>;

const int MIN_CAPTURE_SIZE = 128;
const int MAX_CAPTURE_SIZE = 1024;
const int MAX_LATENCY = 3000;

const std::vector<int> kCaptureSizeValues = {MIN_CAPTURE_SIZE - 1, MIN_CAPTURE_SIZE,
                                             MAX_CAPTURE_SIZE, MAX_CAPTURE_SIZE + 1};
const std::vector<Visualizer::ScalingMode> kScalingModeValues = {
        Visualizer::ScalingMode::NORMALIZED, Visualizer::ScalingMode::AS_PLAYED};
const std::vector<Visualizer::MeasurementMode> kMeasurementModeValues = {
        Visualizer::MeasurementMode::NONE, Visualizer::MeasurementMode::PEAK_RMS};
const std::vector<int> kLatencyValues = {-1, 0, MAX_LATENCY, MAX_LATENCY + 1};

class VisualizerParamTest : public ::testing::TestWithParam<VisualizerParamTestParam>,
                            public EffectHelper {
  public:
    VisualizerParamTest()
        : mCaptureSize(std::get<PARAM_CAPTURE_SIZE>(GetParam())),
          mScalingMode(std::get<PARAM_SCALING_MODE>(GetParam())),
          mMeasurementMode(std::get<PARAM_MEASUREMENT_MODE>(GetParam())),
          mLatency(std::get<PARAM_LATENCY>(GetParam())) {
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
        Visualizer vs = Visualizer::make<Visualizer::captureSizeBytes>(MIN_CAPTURE_SIZE);
        Parameter::Specific specific =
                Parameter::Specific::make<Parameter::Specific::visualizer>(vs);
        return specific;
    }

    static const long kInputFrameCount = 0x100, kOutputFrameCount = 0x100;
    std::shared_ptr<IFactory> mFactory;
    std::shared_ptr<IEffect> mEffect;
    Descriptor mDescriptor;
    int mCaptureSize = MAX_CAPTURE_SIZE;
    Visualizer::ScalingMode mScalingMode = Visualizer::ScalingMode::NORMALIZED;
    Visualizer::MeasurementMode mMeasurementMode = Visualizer::MeasurementMode::NONE;
    int mLatency = 0;

    void SetAndGetCommonParameters() {
        for (auto& it : mCommonTags) {
            auto& tag = it.first;
            auto& vs = it.second;

            // validate parameter
            Descriptor desc;
            ASSERT_STATUS(EX_NONE, mEffect->getDescriptor(&desc));
            const bool valid = isTagInRange(tag, vs, desc);
            const binder_exception_t expected = valid ? EX_NONE : EX_ILLEGAL_ARGUMENT;

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
                EXPECT_STATUS(EX_NONE, mEffect->getParameter(id, &getParam));

                EXPECT_EQ(expectParam, getParam) << "\nexpect:" << expectParam.toString()
                                                 << "\ngetParam:" << getParam.toString();
            }
        }
    }

    void SetAndGetSetOnlyParameters() {
        for (auto& it : mSetOnlyParamTags) {
            auto& tag = it.first;
            auto& vs = it.second;

            // validate parameter
            Descriptor desc;
            ASSERT_STATUS(EX_NONE, mEffect->getDescriptor(&desc));
            const bool valid = isSetOnlyParamTagInRange(tag, vs, desc);
            const binder_exception_t expected = valid ? EX_NONE : EX_ILLEGAL_ARGUMENT;

            // set parameter
            Parameter expectParam;
            Parameter::Specific specific;
            specific.set<Parameter::Specific::visualizer>(vs);
            expectParam.set<Parameter::specific>(specific);
            ASSERT_STATUS(expected, mEffect->setParameter(expectParam));

            //  parameter defined in this setOnlyParameter union must be settable via
            //  setParameter(), but must not be gettable
            Parameter getParam;
            Parameter::Id id;
            Visualizer::Id vsId;
            vsId.set<Visualizer::Id::setOnlyParamTag>(tag);
            id.set<Parameter::Id::visualizerTag>(vsId);
            EXPECT_STATUS(EX_ILLEGAL_ARGUMENT, mEffect->getParameter(id, &getParam));
        }
    }

    void GetandSetGetOnlyParameters() {
        for (auto& tag : mGetOnlyParamTags) {
            // get parameter
            Parameter getParam;
            Parameter::Id id;
            Visualizer::Id vsId;
            vsId.set<Visualizer::Id::getOnlyParamTag>(tag);
            id.set<Parameter::Id::visualizerTag>(vsId);
            ASSERT_STATUS(EX_NONE, mEffect->getParameter(id, &getParam));

            //  parameter defined in this getOnlyParameter union must be gettable via
            //  getParameter(), but must not be settable
            // set parameter
            ASSERT_STATUS(EX_ILLEGAL_ARGUMENT, mEffect->setParameter(getParam));
        }
    }

    void addCaptureSizeParam(int captureSize) {
        Visualizer vs;
        vs.set<Visualizer::captureSizeBytes>(captureSize);
        mCommonTags.push_back({Visualizer::captureSizeBytes, vs});
    }

    void addScalingModeParam(Visualizer::ScalingMode scalingMode) {
        Visualizer vs;
        vs.set<Visualizer::scalingMode>(scalingMode);
        mCommonTags.push_back({Visualizer::scalingMode, vs});
    }

    void addMeasurementModeParam(Visualizer::MeasurementMode measurementMode) {
        Visualizer vs;
        vs.set<Visualizer::measurementMode>(measurementMode);
        mCommonTags.push_back({Visualizer::measurementMode, vs});
    }

    void addLatencyParam(int latency) {
        Visualizer vs;
        Visualizer::SetOnlyParameters setOnlyParam;
        setOnlyParam.set<Visualizer::SetOnlyParameters::latencyMs>(latency);
        vs.set<Visualizer::setOnlyParameters>(setOnlyParam);
        mSetOnlyParamTags.push_back({Visualizer::SetOnlyParameters::latencyMs, vs});
    }

    void addMeasurementTag() {
        mGetOnlyParamTags.push_back(Visualizer::GetOnlyParameters::measurement);
    }

    void addCaptureBytesTag() {
        mGetOnlyParamTags.push_back(Visualizer::GetOnlyParameters::captureBytes);
    }

    bool isTagInRange(const Visualizer::Tag& tag, const Visualizer& vs,
                      const Descriptor& desc) const {
        const Visualizer::Capability& vsCap = desc.capability.get<Capability::visualizer>();
        switch (tag) {
            case Visualizer::captureSizeBytes: {
                int captureSize = vs.get<Visualizer::captureSizeBytes>();
                return isCaptureSizeInRange(vsCap, captureSize);
            }
            case Visualizer::scalingMode:
            case Visualizer::measurementMode:
                return true;
            default:
                return false;
        }
    }

    bool isSetOnlyParamTagInRange(Visualizer::SetOnlyParameters::Tag, const Visualizer& vs,
                                  const Descriptor& desc) const {
        const Visualizer::Capability& vsCap = desc.capability.get<Capability::visualizer>();
        if (vs.getTag() != Visualizer::setOnlyParameters) return false;
        Visualizer::SetOnlyParameters setOnlyParam = vs.get<Visualizer::setOnlyParameters>();
        if (setOnlyParam.getTag() != Visualizer::SetOnlyParameters::latencyMs) return false;
        int latency = setOnlyParam.get<Visualizer::SetOnlyParameters::latencyMs>();
        return isLatencyInRange(vsCap, latency);
    }

    bool isCaptureSizeInRange(const Visualizer::Capability& cap, int captureSize) const {
        return (captureSize >= cap.captureSizeRange.minBytes &&
                captureSize <= cap.captureSizeRange.maxBytes);
    }

    bool isLatencyInRange(const Visualizer::Capability& cap, int latency) const {
        return (latency >= 0 && latency <= cap.maxLatencyMs);
    }

  private:
    std::vector<std::pair<Visualizer::Tag, Visualizer>> mCommonTags;
    std::vector<std::pair<Visualizer::SetOnlyParameters::Tag, Visualizer>> mSetOnlyParamTags;
    std::vector<Visualizer::GetOnlyParameters::Tag> mGetOnlyParamTags;
    void CleanUp() {
        mCommonTags.clear();
        mSetOnlyParamTags.clear();
        mGetOnlyParamTags.clear();
    }
};

TEST_P(VisualizerParamTest, SetAndGetCaptureSize) {
    EXPECT_NO_FATAL_FAILURE(addCaptureSizeParam(mCaptureSize));
    SetAndGetCommonParameters();
}

TEST_P(VisualizerParamTest, SetAndGetScalingMode) {
    EXPECT_NO_FATAL_FAILURE(addScalingModeParam(mScalingMode));
    SetAndGetCommonParameters();
}

TEST_P(VisualizerParamTest, SetAndGetMeasurementMode) {
    EXPECT_NO_FATAL_FAILURE(addMeasurementModeParam(mMeasurementMode));
    SetAndGetCommonParameters();
}

TEST_P(VisualizerParamTest, SetAndGetLatency) {
    EXPECT_NO_FATAL_FAILURE(addLatencyParam(mLatency));
    SetAndGetSetOnlyParameters();
}

TEST_P(VisualizerParamTest, GetAndSetMeasurement) {
    EXPECT_NO_FATAL_FAILURE(addMeasurementTag());
    GetandSetGetOnlyParameters();
}

TEST_P(VisualizerParamTest, GetAndSetCaptureBytes) {
    EXPECT_NO_FATAL_FAILURE(addCaptureBytesTag());
    GetandSetGetOnlyParameters();
}

INSTANTIATE_TEST_SUITE_P(
        VisualizerTest, VisualizerParamTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, kVisualizerTypeUUID)),
                           testing::ValuesIn(kCaptureSizeValues),
                           testing::ValuesIn(kScalingModeValues),
                           testing::ValuesIn(kMeasurementModeValues),
                           testing::ValuesIn(kLatencyValues)),
        [](const testing::TestParamInfo<VisualizerParamTest::ParamType>& info) {
            auto descriptor = std::get<PARAM_INSTANCE_NAME>(info.param).second;
            std::string captureSize = std::to_string(std::get<PARAM_CAPTURE_SIZE>(info.param));
            std::string scalingMode =
                    std::to_string(static_cast<int>(std::get<PARAM_SCALING_MODE>(info.param)));
            std::string measurementMode =
                    std::to_string(static_cast<int>(std::get<PARAM_MEASUREMENT_MODE>(info.param)));
            std::string latency = std::to_string(std::get<PARAM_LATENCY>(info.param));

            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               descriptor.common.id.uuid.toString() + "_captureSize" + captureSize +
                               "_scalingMode" + scalingMode + "_measurementMode" + measurementMode +
                               "_latency" + latency;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(VisualizerParamTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}