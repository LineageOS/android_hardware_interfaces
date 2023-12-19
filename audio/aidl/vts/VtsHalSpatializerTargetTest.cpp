/*
 * Copyright (C) 2023 The Android Open Source Project
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

#define LOG_TAG "VtsHalSpatializerTest"
#include <android-base/logging.h>

#include "EffectHelper.h"

using namespace android;

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::getEffectTypeUuidSpatializer;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;
using aidl::android::hardware::audio::effect::Range;
using aidl::android::hardware::audio::effect::Spatializer;
using aidl::android::media::audio::common::HeadTracking;
using aidl::android::media::audio::common::Spatialization;
using android::hardware::audio::common::testing::detail::TestExecutionTracer;
using ::android::internal::ToString;

enum ParamName {
    PARAM_INSTANCE_NAME,
    PARAM_SPATIALIZATION_LEVEL,
    PARAM_SPATIALIZATION_MODE,
    PARAM_HEADTRACK_SENSORID,
    PARAM_HEADTRACK_MODE,
    PARAM_HEADTRACK_CONNECTION_MODE
};

using SpatializerParamTestParam =
        std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, Spatialization::Level,
                   Spatialization::Mode, int /* sensor ID */, HeadTracking::Mode,
                   HeadTracking::ConnectionMode>;

class SpatializerParamTest : public ::testing::TestWithParam<SpatializerParamTestParam>,
                             public EffectHelper {
  public:
    SpatializerParamTest()
        : mSpatializerParams([&]() {
              Spatialization::Level level = std::get<PARAM_SPATIALIZATION_LEVEL>(GetParam());
              Spatialization::Mode mode = std::get<PARAM_SPATIALIZATION_MODE>(GetParam());
              int sensorId = std::get<PARAM_HEADTRACK_SENSORID>(GetParam());
              HeadTracking::Mode htMode = std::get<PARAM_HEADTRACK_MODE>(GetParam());
              HeadTracking::ConnectionMode htConnectMode =
                      std::get<PARAM_HEADTRACK_CONNECTION_MODE>(GetParam());
              std::map<Spatializer::Tag, Spatializer> params;
              params[Spatializer::spatializationLevel] =
                      Spatializer::make<Spatializer::spatializationLevel>(level);
              params[Spatializer::spatializationMode] =
                      Spatializer::make<Spatializer::spatializationMode>(mode);
              params[Spatializer::headTrackingSensorId] =
                      Spatializer::make<Spatializer::headTrackingSensorId>(sensorId);
              params[Spatializer::headTrackingMode] =
                      Spatializer::make<Spatializer::headTrackingMode>(htMode);
              params[Spatializer::headTrackingConnectionMode] =
                      Spatializer::make<Spatializer::headTrackingConnectionMode>(htConnectMode);
              return params;
          }()) {
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
        Spatializer spatializer = Spatializer::make<Spatializer::headTrackingSensorId>(0);
        Parameter::Specific specific =
                Parameter::Specific::make<Parameter::Specific::spatializer>(spatializer);
        return specific;
    }

    static const long kInputFrameCount = 0x100, kOutputFrameCount = 0x100;
    std::shared_ptr<IFactory> mFactory;
    std::shared_ptr<IEffect> mEffect;
    Descriptor mDescriptor;
    const std::map<Spatializer::Tag, Spatializer> mSpatializerParams;
};

TEST_P(SpatializerParamTest, SetAndGetParam) {
    for (const auto& it : mSpatializerParams) {
        auto& tag = it.first;
        auto& spatializer = it.second;

        // validate parameter
        Descriptor desc;
        ASSERT_STATUS(EX_NONE, mEffect->getDescriptor(&desc));
        const bool valid = isParameterValid<Spatializer, Range::spatializer>(it.second, desc);
        const binder_exception_t expected = valid ? EX_NONE : EX_ILLEGAL_ARGUMENT;

        // set parameter
        Parameter expectParam;
        Parameter::Specific specific;
        specific.set<Parameter::Specific::spatializer>(spatializer);
        expectParam.set<Parameter::specific>(specific);
        EXPECT_STATUS(expected, mEffect->setParameter(expectParam)) << expectParam.toString();

        // only get if parameter in range and set success
        if (expected == EX_NONE) {
            Parameter getParam;
            Parameter::Id id;
            Spatializer::Id spatializerId;
            spatializerId.set<Spatializer::Id::commonTag>(tag);
            id.set<Parameter::Id::spatializerTag>(spatializerId);
            // if set success, then get should match
            EXPECT_STATUS(expected, mEffect->getParameter(id, &getParam));
            EXPECT_EQ(expectParam, getParam);
        }
    }
}

std::vector<std::pair<std::shared_ptr<IFactory>, Descriptor>> kDescPair;
INSTANTIATE_TEST_SUITE_P(
        SpatializerTest, SpatializerParamTest,
        ::testing::Combine(
                testing::ValuesIn(kDescPair = EffectFactoryHelper::getAllEffectDescriptors(
                                          IFactory::descriptor, getEffectTypeUuidSpatializer())),
                testing::ValuesIn(EffectHelper::getTestValueSet<
                                  Spatializer, Spatialization::Level, Range::spatializer,
                                  Spatializer::spatializationLevel>(kDescPair)),
                testing::ValuesIn(EffectHelper::getTestValueSet<
                                  Spatializer, Spatialization::Mode, Range::spatializer,
                                  Spatializer::spatializationMode>(kDescPair)),
                testing::ValuesIn(
                        EffectHelper::getTestValueSet<Spatializer, int, Range::spatializer,
                                                      Spatializer::headTrackingSensorId>(
                                kDescPair, EffectHelper::expandTestValueBasic<int>)),
                testing::ValuesIn(EffectHelper::getTestValueSet<
                                  Spatializer, HeadTracking::Mode, Range::spatializer,
                                  Spatializer::headTrackingMode>(kDescPair)),
                testing::ValuesIn(EffectHelper::getTestValueSet<
                                  Spatializer, HeadTracking::ConnectionMode, Range::spatializer,
                                  Spatializer::headTrackingConnectionMode>(kDescPair))),
        [](const testing::TestParamInfo<SpatializerParamTest::ParamType>& info) {
            auto descriptor = std::get<PARAM_INSTANCE_NAME>(info.param).second;
            std::string level = ToString(std::get<PARAM_SPATIALIZATION_LEVEL>(info.param));
            std::string mode = ToString(std::get<PARAM_SPATIALIZATION_MODE>(info.param));
            std::string sensorId = ToString(std::get<PARAM_HEADTRACK_SENSORID>(info.param));
            std::string htMode = ToString(std::get<PARAM_HEADTRACK_MODE>(info.param));
            std::string htConnectMode =
                    ToString(std::get<PARAM_HEADTRACK_CONNECTION_MODE>(info.param));
            std::string name = getPrefix(descriptor) + "_sensorID_" + level + "_mode_" + mode +
                               "_sensorID_" + sensorId + "_HTMode_" + htMode +
                               "_HTConnectionMode_" + htConnectMode;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(SpatializerParamTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::UnitTest::GetInstance()->listeners().Append(new TestExecutionTracer());
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
