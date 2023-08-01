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
#define LOG_TAG "VtsHalVolumeTest"
#include <android-base/logging.h>

#include "EffectHelper.h"

using namespace android;

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::getEffectTypeUuidVolume;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;
using aidl::android::hardware::audio::effect::Volume;

/**
 * Here we focus on specific parameter checking, general IEffect interfaces testing performed in
 * VtsAudioEffectTargetTest.
 */
enum ParamName { PARAM_INSTANCE_NAME, PARAM_LEVEL, PARAM_MUTE };
using VolumeParamTestParam =
        std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int, bool>;

class VolumeParamTest : public ::testing::TestWithParam<VolumeParamTestParam>, public EffectHelper {
  public:
    VolumeParamTest()
        : mParamLevel(std::get<PARAM_LEVEL>(GetParam())),
          mParamMute(std::get<PARAM_MUTE>(GetParam())) {
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
        Volume vol = Volume::make<Volume::levelDb>(-9600);
        Parameter::Specific specific = Parameter::Specific::make<Parameter::Specific::volume>(vol);
        return specific;
    }

    static const long kInputFrameCount = 0x100, kOutputFrameCount = 0x100;
    std::shared_ptr<IFactory> mFactory;
    std::shared_ptr<IEffect> mEffect;
    Descriptor mDescriptor;
    int mParamLevel = 0;
    bool mParamMute = false;

    void SetAndGetParameters() {
        for (auto& it : mTags) {
            auto& tag = it.first;
            auto& vol = it.second;

            // validate parameter
            Descriptor desc;
            ASSERT_STATUS(EX_NONE, mEffect->getDescriptor(&desc));
            const bool valid = isParameterValid<Volume, Range::volume>(it.second, desc);
            const binder_exception_t expected = valid ? EX_NONE : EX_ILLEGAL_ARGUMENT;

            // set parameter
            Parameter expectParam;
            Parameter::Specific specific;
            specific.set<Parameter::Specific::volume>(vol);
            expectParam.set<Parameter::specific>(specific);
            EXPECT_STATUS(expected, mEffect->setParameter(expectParam)) << expectParam.toString();

            // only get if parameter is in range and set success
            if (expected == EX_NONE) {
                Parameter getParam;
                Parameter::Id id;
                Volume::Id volId;
                volId.set<Volume::Id::commonTag>(tag);
                id.set<Parameter::Id::volumeTag>(volId);
                EXPECT_STATUS(EX_NONE, mEffect->getParameter(id, &getParam));

                EXPECT_EQ(expectParam, getParam) << "\nexpect:" << expectParam.toString()
                                                 << "\ngetParam:" << getParam.toString();
            }
        }
    }

    void addLevelParam(int level) {
        Volume vol;
        vol.set<Volume::levelDb>(level);
        mTags.push_back({Volume::levelDb, vol});
    }

    void addMuteParam(bool mute) {
        Volume vol;
        vol.set<Volume::mute>(mute);
        mTags.push_back({Volume::mute, vol});
    }

  private:
    std::vector<std::pair<Volume::Tag, Volume>> mTags;
    void CleanUp() { mTags.clear(); }
};

TEST_P(VolumeParamTest, SetAndGetLevel) {
    EXPECT_NO_FATAL_FAILURE(addLevelParam(mParamLevel));
    SetAndGetParameters();
}

TEST_P(VolumeParamTest, SetAndGetMute) {
    EXPECT_NO_FATAL_FAILURE(addMuteParam(mParamMute));
    SetAndGetParameters();
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

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
