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

#include <aidl/Vintf.h>
#include <aidl/android/hardware/audio/effect/NoiseSuppression.h>
#define LOG_TAG "VtsHalNSParamTest"
#include <android-base/logging.h>
#include <android/binder_enums.h>

#include "EffectHelper.h"

using namespace android;

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::getEffectTypeUuidNoiseSuppression;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::NoiseSuppression;
using aidl::android::hardware::audio::effect::Parameter;
using android::hardware::audio::common::testing::detail::TestExecutionTracer;

enum ParamName { PARAM_INSTANCE_NAME, PARAM_LEVEL, PARAM_TYPE };
using NSParamTestParam = std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>,
                                    NoiseSuppression::Level, NoiseSuppression::Type>;

class NSParamTest : public ::testing::TestWithParam<NSParamTestParam>, public EffectHelper {
  public:
    NSParamTest()
        : mLevel(std::get<PARAM_LEVEL>(GetParam())), mType(std::get<PARAM_TYPE>(GetParam())) {
        std::tie(mFactory, mDescriptor) = std::get<PARAM_INSTANCE_NAME>(GetParam());
    }

    void SetUp() override {
        ASSERT_NE(nullptr, mFactory);
        ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));

        std::optional<Parameter::Specific> specific = getDefaultParamSpecific();
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

    std::optional<Parameter::Specific> getDefaultParamSpecific() {
        NoiseSuppression ns =
                NoiseSuppression::make<NoiseSuppression::level>(NoiseSuppression::Level::MEDIUM);
        if (!isParameterValid<NoiseSuppression, Range::noiseSuppression>(ns, mDescriptor)) {
            return std::nullopt;
        }

        Parameter::Specific specific =
                Parameter::Specific::make<Parameter::Specific::noiseSuppression>(ns);
        return specific;
    }

    static const long kInputFrameCount = 0x100, kOutputFrameCount = 0x100;
    std::shared_ptr<IFactory> mFactory;
    std::shared_ptr<IEffect> mEffect;
    Descriptor mDescriptor;
    NoiseSuppression::Level mLevel;
    NoiseSuppression::Type mType;

    void SetAndGetParameters() {
        for (auto& it : mTags) {
            auto& tag = it.first;
            auto& ns = it.second;

            // validate parameter
            Descriptor desc;
            ASSERT_STATUS(EX_NONE, mEffect->getDescriptor(&desc));
            const bool valid =
                    isParameterValid<NoiseSuppression, Range::noiseSuppression>(ns, desc);
            const binder_exception_t expected = valid ? EX_NONE : EX_ILLEGAL_ARGUMENT;

            // set parameter
            Parameter expectParam;
            Parameter::Specific specific;
            specific.set<Parameter::Specific::noiseSuppression>(ns);
            expectParam.set<Parameter::specific>(specific);
            EXPECT_STATUS(expected, mEffect->setParameter(expectParam)) << expectParam.toString();

            // only get if parameter in range and set success
            if (expected == EX_NONE) {
                Parameter getParam;
                Parameter::Id id;
                NoiseSuppression::Id specificId;
                specificId.set<NoiseSuppression::Id::commonTag>(tag);
                id.set<Parameter::Id::noiseSuppressionTag>(specificId);
                EXPECT_STATUS(EX_NONE, mEffect->getParameter(id, &getParam));

                EXPECT_EQ(expectParam, getParam) << "\nexpect:" << expectParam.toString()
                                                 << "\ngetParam:" << getParam.toString();
            }
        }
    }

    void addLevelParam(NoiseSuppression::Level level) {
        NoiseSuppression ns;
        ns.set<NoiseSuppression::level>(level);
        mTags.push_back({NoiseSuppression::level, ns});
    }
    void addTypeParam(NoiseSuppression::Type type) {
        NoiseSuppression ns;
        ns.set<NoiseSuppression::type>(type);
        mTags.push_back({NoiseSuppression::type, ns});
    }
    static std::unordered_set<NoiseSuppression::Level> getLevelValues() {
        return {ndk::enum_range<NoiseSuppression::Level>().begin(),
                ndk::enum_range<NoiseSuppression::Level>().end()};
    }
    static std::unordered_set<NoiseSuppression::Type> getTypeValues() {
        return {ndk::enum_range<NoiseSuppression::Type>().begin(),
                ndk::enum_range<NoiseSuppression::Type>().end()};
    }

  private:
    std::vector<std::pair<NoiseSuppression::Tag, NoiseSuppression>> mTags;
    void CleanUp() { mTags.clear(); }
};

TEST_P(NSParamTest, SetAndGetLevel) {
    EXPECT_NO_FATAL_FAILURE(addLevelParam(mLevel));
    SetAndGetParameters();
}

TEST_P(NSParamTest, SetAndGetType) {
    EXPECT_NO_FATAL_FAILURE(addLevelParam(mLevel));
    SetAndGetParameters();
}

INSTANTIATE_TEST_SUITE_P(
        NSParamTest, NSParamTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidNoiseSuppression())),
                           testing::ValuesIn(NSParamTest::getLevelValues()),
                           testing::ValuesIn(NSParamTest::getTypeValues())),
        [](const testing::TestParamInfo<NSParamTest::ParamType>& info) {
            auto descriptor = std::get<PARAM_INSTANCE_NAME>(info.param).second;
            std::string level = aidl::android::hardware::audio::effect::toString(
                    std::get<PARAM_LEVEL>(info.param));
            std::string type = aidl::android::hardware::audio::effect::toString(
                    std::get<PARAM_TYPE>(info.param));
            std::string name = getPrefix(descriptor) + "_level_" + level + "_type_" + type;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(NSParamTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::UnitTest::GetInstance()->listeners().Append(new TestExecutionTracer());
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
