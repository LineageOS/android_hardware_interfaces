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
#define LOG_TAG "VtsHalDownmixTargetTest"
#include <android-base/logging.h>

#include "EffectHelper.h"

using namespace android;

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::Downmix;
using aidl::android::hardware::audio::effect::getEffectTypeUuidDownmix;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;

/**
 * Here we focus on specific parameter checking, general IEffect interfaces testing performed in
 * VtsAudioEffectTargetTest.
 */
enum ParamName { PARAM_INSTANCE_NAME, PARAM_TYPE };
using DownmixParamTestParam =
        std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, Downmix::Type>;

// Testing for enum values
const std::vector<Downmix::Type> kTypeValues = {Downmix::Type::STRIP, Downmix::Type::FOLD};

class DownmixParamTest : public ::testing::TestWithParam<DownmixParamTestParam>,
                         public EffectHelper {
  public:
    DownmixParamTest() : mParamType(std::get<PARAM_TYPE>(GetParam())) {
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
    Downmix::Type mParamType = Downmix::Type::STRIP;

    void SetAndGetDownmixParameters() {
        for (auto& it : mTags) {
            auto& tag = it.first;
            auto& dm = it.second;

            // set parameter
            Parameter expectParam;
            Parameter::Specific specific;
            specific.set<Parameter::Specific::downmix>(dm);
            expectParam.set<Parameter::specific>(specific);
            // All values are valid, set parameter should succeed
            EXPECT_STATUS(EX_NONE, mEffect->setParameter(expectParam)) << expectParam.toString();

            // get parameter
            Parameter getParam;
            Parameter::Id id;
            Downmix::Id dmId;
            dmId.set<Downmix::Id::commonTag>(tag);
            id.set<Parameter::Id::downmixTag>(dmId);
            EXPECT_STATUS(EX_NONE, mEffect->getParameter(id, &getParam));

            EXPECT_EQ(expectParam, getParam);
        }
    }

    void addTypeParam(Downmix::Type type) {
        Downmix dm;
        dm.set<Downmix::type>(type);
        mTags.push_back({Downmix::type, dm});
    }

    Parameter::Specific getDefaultParamSpecific() {
        Downmix dm = Downmix::make<Downmix::type>(Downmix::Type::STRIP);
        Parameter::Specific specific = Parameter::Specific::make<Parameter::Specific::downmix>(dm);
        return specific;
    }

  private:
    std::vector<std::pair<Downmix::Tag, Downmix>> mTags;
    void CleanUp() { mTags.clear(); }
};

TEST_P(DownmixParamTest, SetAndGetType) {
    EXPECT_NO_FATAL_FAILURE(addTypeParam(mParamType));
    SetAndGetDownmixParameters();
}

INSTANTIATE_TEST_SUITE_P(
        DownmixTest, DownmixParamTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidDownmix())),
                           testing::ValuesIn(kTypeValues)),
        [](const testing::TestParamInfo<DownmixParamTest::ParamType>& info) {
            auto descriptor = std::get<PARAM_INSTANCE_NAME>(info.param).second;
            std::string type = std::to_string(static_cast<int>(std::get<PARAM_TYPE>(info.param)));
            std::string name = getPrefix(descriptor) + "_type" + type;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DownmixParamTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
