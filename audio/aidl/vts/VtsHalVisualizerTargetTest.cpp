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
#include <android/binder_enums.h>
#include <unordered_set>

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
        Visualizer vs = Visualizer::make<Visualizer::captureSamples>(
                mDescriptor.capability.get<Capability::visualizer>().captureSampleRange.max);
        Parameter::Specific specific =
                Parameter::Specific::make<Parameter::Specific::visualizer>(vs);
        return specific;
    }

    static const long kInputFrameCount = 0x100, kOutputFrameCount = 0x100;
    std::shared_ptr<IFactory> mFactory;
    std::shared_ptr<IEffect> mEffect;
    Descriptor mDescriptor;
    int mCaptureSize;
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
            const bool valid = isSetOnlyParamTagInRange(vs, desc);
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
        vs.set<Visualizer::captureSamples>(captureSize);
        mCommonTags.push_back({Visualizer::captureSamples, vs});
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
        mGetOnlyParamTags.push_back(Visualizer::GetOnlyParameters::captureSampleBuffer);
    }

    bool isTagInRange(const Visualizer::Tag& tag, const Visualizer& vs,
                      const Descriptor& desc) const {
        const Visualizer::Capability& vsCap = desc.capability.get<Capability::visualizer>();
        switch (tag) {
            case Visualizer::captureSamples: {
                int captureSize = vs.get<Visualizer::captureSamples>();
                return captureSize >= vsCap.captureSampleRange.min &&
                       captureSize <= vsCap.captureSampleRange.max;
            }
            case Visualizer::scalingMode:
            case Visualizer::measurementMode:
                return true;
            case Visualizer::setOnlyParameters: {
                auto setOnly = vs.get<Visualizer::setOnlyParameters>();
                if (setOnly.getTag() != Visualizer::SetOnlyParameters::latencyMs) {
                    return false;
                }
                auto latencyMs = setOnly.get<Visualizer::SetOnlyParameters::latencyMs>();
                return latencyMs >= 0 && latencyMs <= vsCap.maxLatencyMs;
            }
            default:
                return false;
        }
    }

    bool isSetOnlyParamTagInRange(const Visualizer& vs, const Descriptor& desc) const {
        const Visualizer::Capability& vsCap = desc.capability.get<Capability::visualizer>();
        if (vs.getTag() != Visualizer::setOnlyParameters) return false;
        Visualizer::SetOnlyParameters setOnlyParam = vs.get<Visualizer::setOnlyParameters>();
        if (setOnlyParam.getTag() != Visualizer::SetOnlyParameters::latencyMs) return false;
        int latency = setOnlyParam.get<Visualizer::SetOnlyParameters::latencyMs>();
        return isLatencyInRange(vsCap, latency);
    }

    bool isLatencyInRange(const Visualizer::Capability& cap, int latency) const {
        return (latency >= 0 && latency <= cap.maxLatencyMs);
    }

    static std::unordered_set<int> getCaptureSizeValues() {
        auto descList = EffectFactoryHelper::getAllEffectDescriptors(IFactory::descriptor,
                                                                     kVisualizerTypeUUID);
        int minCaptureSize = std::numeric_limits<int>::max();
        int maxCaptureSize = std::numeric_limits<int>::min();
        for (const auto& it : descList) {
            maxCaptureSize = std::max(
                    it.second.capability.get<Capability::visualizer>().captureSampleRange.max,
                    maxCaptureSize);
            minCaptureSize = std::min(
                    it.second.capability.get<Capability ::visualizer>().captureSampleRange.min,
                    minCaptureSize);
        }
        return {std::numeric_limits<int>::min(),        minCaptureSize - 1, minCaptureSize,
                (minCaptureSize + maxCaptureSize) >> 1, maxCaptureSize,     maxCaptureSize + 1,
                std::numeric_limits<int>::max()};
    }

    static std::unordered_set<int> getLatencyValues() {
        auto descList = EffectFactoryHelper::getAllEffectDescriptors(IFactory::descriptor,
                                                                     kVisualizerTypeUUID);
        const auto max = std::max_element(
                descList.begin(), descList.end(),
                [](const std::pair<std::shared_ptr<IFactory>, Descriptor>& a,
                   const std::pair<std::shared_ptr<IFactory>, Descriptor>& b) {
                    return a.second.capability.get<Capability::visualizer>().maxLatencyMs <
                           b.second.capability.get<Capability::visualizer>().maxLatencyMs;
                });
        if (max == descList.end()) {
            return {0};
        }
        int maxDelay = max->second.capability.get<Capability::visualizer>().maxLatencyMs;
        return {-1, 0, maxDelay >> 1, maxDelay - 1, maxDelay, maxDelay + 1};
    }
    static std::unordered_set<Visualizer::MeasurementMode> getMeasurementModeValues() {
        return {ndk::enum_range<Visualizer::MeasurementMode>().begin(),
                ndk::enum_range<Visualizer::MeasurementMode>().end()};
    }
    static std::unordered_set<Visualizer::ScalingMode> getScalingModeValues() {
        return {ndk::enum_range<Visualizer::ScalingMode>().begin(),
                ndk::enum_range<Visualizer::ScalingMode>().end()};
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
        VisualizerParamTest, VisualizerParamTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, kVisualizerTypeUUID)),
                           testing::ValuesIn(VisualizerParamTest::getCaptureSizeValues()),
                           testing::ValuesIn(VisualizerParamTest::getScalingModeValues()),
                           testing::ValuesIn(VisualizerParamTest::getMeasurementModeValues()),
                           testing::ValuesIn(VisualizerParamTest::getLatencyValues())),
        [](const testing::TestParamInfo<VisualizerParamTest::ParamType>& info) {
            auto descriptor = std::get<PARAM_INSTANCE_NAME>(info.param).second;
            std::string captureSize = std::to_string(std::get<PARAM_CAPTURE_SIZE>(info.param));
            std::string scalingMode = aidl::android::hardware::audio::effect::toString(
                    std::get<PARAM_SCALING_MODE>(info.param));
            std::string measurementMode = aidl::android::hardware::audio::effect::toString(
                    std::get<PARAM_MEASUREMENT_MODE>(info.param));
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