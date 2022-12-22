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

#define LOG_TAG "VtsHalHapticGeneratorTargetTest"

#include <Utils.h>
#include <aidl/Vintf.h>
#include <android/binder_enums.h>
#include <unordered_set>

#include "EffectHelper.h"

using namespace android;

using aidl::android::hardware::audio::effect::Capability;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::HapticGenerator;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::kHapticGeneratorTypeUUID;
using aidl::android::hardware::audio::effect::Parameter;

/**
 * Here we focus on specific parameter checking, general IEffect interfaces testing performed in
 * VtsAudioEffectTargetTest.
 */
enum ParamName {
    PARAM_INSTANCE_NAME,
    PARAM_HAPTIC_SCALE_ID,
    PARAM_HAPTIC_SCALE_VIBRATOR_SCALE,
    PARAM_VIBRATION_INFORMATION_RESONANT_FREQUENCY,
    PARAM_VIBRATION_INFORMATION_Q_FACTOR,
    PARAM_VIBRATION_INFORMATION_MAX_AMPLITUDE,
};
using HapticGeneratorParamTestParam =
        std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int,
                   HapticGenerator::VibratorScale, float, float, float>;

/*
 * Testing parameter range, assuming the parameter supported by effect is in this range.
 * Parameter should be within the valid range defined in the documentation,
 * for any supported value test expects EX_NONE from IEffect.setParameter(),
 * otherwise expect EX_ILLEGAL_ARGUMENT.
 */

// TODO : Update the test values once range/capability is updated by implementation
const int MIN_ID = std::numeric_limits<int>::min();
const int MAX_ID = std::numeric_limits<int>::max();
const float MIN_FLOAT = std::numeric_limits<float>::min();
const float MAX_FLOAT = std::numeric_limits<float>::max();

const std::vector<int> kHapticScaleIdValues = {MIN_ID, 0, MAX_ID};
const std::vector<HapticGenerator::VibratorScale> kVibratorScaleValues = {
        ndk::enum_range<HapticGenerator::VibratorScale>().begin(),
        ndk::enum_range<HapticGenerator::VibratorScale>().end()};

const std::vector<float> kResonantFrequencyValues = {MIN_FLOAT, 100, MAX_FLOAT};
const std::vector<float> kQFactorValues = {MIN_FLOAT, 100, MAX_FLOAT};
const std::vector<float> kMaxAmplitude = {MIN_FLOAT, 100, MAX_FLOAT};

class HapticGeneratorParamTest : public ::testing::TestWithParam<HapticGeneratorParamTestParam>,
                                 public EffectHelper {
  public:
    HapticGeneratorParamTest()
        : mParamHapticScaleId(std::get<PARAM_HAPTIC_SCALE_ID>(GetParam())),
          mParamVibratorScale(std::get<PARAM_HAPTIC_SCALE_VIBRATOR_SCALE>(GetParam())),
          mParamResonantFrequency(
                  std::get<PARAM_VIBRATION_INFORMATION_RESONANT_FREQUENCY>(GetParam())),
          mParamQFactor(std::get<PARAM_VIBRATION_INFORMATION_Q_FACTOR>(GetParam())),
          mParamMaxAmplitude(std::get<PARAM_VIBRATION_INFORMATION_MAX_AMPLITUDE>(GetParam())) {
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
        HapticGenerator::HapticScale hapticScale = {.id = 0,
                                                    .scale = HapticGenerator::VibratorScale::MUTE};
        HapticGenerator hg = HapticGenerator::make<HapticGenerator::hapticScale>(hapticScale);
        Parameter::Specific specific =
                Parameter::Specific::make<Parameter::Specific::hapticGenerator>(hg);
        return specific;
    }

    static const long kInputFrameCount = 0x100, kOutputFrameCount = 0x100;
    std::shared_ptr<IFactory> mFactory;
    std::shared_ptr<IEffect> mEffect;
    Descriptor mDescriptor;
    int mParamHapticScaleId = 0;
    HapticGenerator::VibratorScale mParamVibratorScale = HapticGenerator::VibratorScale::MUTE;
    float mParamResonantFrequency = 0;
    float mParamQFactor = 0;
    float mParamMaxAmplitude = 0;

    void SetAndGetHapticGeneratorParameters() {
        for (auto& it : mTags) {
            auto& tag = it.first;
            auto& hg = it.second;

            // set parameter
            Parameter expectParam;
            Parameter::Specific specific;
            specific.set<Parameter::Specific::hapticGenerator>(hg);
            expectParam.set<Parameter::specific>(specific);
            EXPECT_STATUS(EX_NONE, mEffect->setParameter(expectParam)) << expectParam.toString();

            // get parameter
            Parameter getParam;
            Parameter::Id id;
            HapticGenerator::Id hgId;
            hgId.set<HapticGenerator::Id::commonTag>(tag);
            id.set<Parameter::Id::hapticGeneratorTag>(hgId);
            EXPECT_STATUS(EX_NONE, mEffect->getParameter(id, &getParam));
            EXPECT_EQ(expectParam, getParam);
        }
    }

    void addHapticScaleParam(int id, HapticGenerator::VibratorScale scale) {
        HapticGenerator hg;
        HapticGenerator::HapticScale hapticScale = {.id = id, .scale = scale};
        hg.set<HapticGenerator::hapticScale>(hapticScale);
        mTags.push_back({HapticGenerator::hapticScale, hg});
    }

    void addVibratorInformationParam(float resonantFrequencyHz, float qFactor, float maxAmplitude) {
        HapticGenerator hg;
        HapticGenerator::VibratorInformation vibrationInfo = {
                .resonantFrequencyHz = resonantFrequencyHz,
                .qFactor = qFactor,
                .maxAmplitude = maxAmplitude};
        hg.set<HapticGenerator::vibratorInfo>(vibrationInfo);
        mTags.push_back({HapticGenerator::vibratorInfo, hg});
    }

  private:
    std::vector<std::pair<HapticGenerator::Tag, HapticGenerator>> mTags;

    void CleanUp() { mTags.clear(); }
};

TEST_P(HapticGeneratorParamTest, SetAndGetHapticScale) {
    EXPECT_NO_FATAL_FAILURE(addHapticScaleParam(mParamHapticScaleId, mParamVibratorScale));
    SetAndGetHapticGeneratorParameters();
}

TEST_P(HapticGeneratorParamTest, SetAndGetVibratorInformation) {
    EXPECT_NO_FATAL_FAILURE(addVibratorInformationParam(mParamResonantFrequency, mParamQFactor,
                                                        mParamMaxAmplitude));
    SetAndGetHapticGeneratorParameters();
}

INSTANTIATE_TEST_SUITE_P(
        HapticGeneratorValidTest, HapticGeneratorParamTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, kHapticGeneratorTypeUUID)),
                           testing::ValuesIn(kHapticScaleIdValues),
                           testing::ValuesIn(kVibratorScaleValues),
                           testing::ValuesIn(kResonantFrequencyValues),
                           testing::ValuesIn(kQFactorValues), testing::ValuesIn(kMaxAmplitude)),
        [](const testing::TestParamInfo<HapticGeneratorParamTest::ParamType>& info) {
            auto descriptor = std::get<PARAM_INSTANCE_NAME>(info.param).second;
            std::string hapticScaleID = std::to_string(std::get<PARAM_HAPTIC_SCALE_ID>(info.param));
            std::string hapticScaleVibScale = std::to_string(
                    static_cast<int>(std::get<PARAM_HAPTIC_SCALE_VIBRATOR_SCALE>(info.param)));
            std::string resonantFrequency = std::to_string(
                    std::get<PARAM_VIBRATION_INFORMATION_RESONANT_FREQUENCY>(info.param));
            std::string qFactor =
                    std::to_string(std::get<PARAM_VIBRATION_INFORMATION_Q_FACTOR>(info.param));
            std::string maxAmplitude =
                    std::to_string(std::get<PARAM_VIBRATION_INFORMATION_MAX_AMPLITUDE>(info.param));
            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               descriptor.common.id.uuid.toString() + "_hapticScaleId" +
                               hapticScaleID + "_hapticScaleVibScale" + hapticScaleVibScale +
                               "_resonantFrequency" + resonantFrequency + "_qFactor" + qFactor +
                               "_maxAmplitude" + maxAmplitude;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

INSTANTIATE_TEST_SUITE_P(
        HapticGeneratorInvalidTest, HapticGeneratorParamTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, kHapticGeneratorTypeUUID)),
                           testing::Values(MIN_ID - 1),
                           testing::Values(HapticGenerator::VibratorScale::MUTE),
                           testing::Values(MIN_FLOAT), testing::Values(MIN_FLOAT),
                           testing::Values(MIN_FLOAT)),
        [](const testing::TestParamInfo<HapticGeneratorParamTest::ParamType>& info) {
            auto descriptor = std::get<PARAM_INSTANCE_NAME>(info.param).second;
            std::string hapticScaleID = std::to_string(std::get<PARAM_HAPTIC_SCALE_ID>(info.param));
            std::string hapticScaleVibScale = std::to_string(
                    static_cast<int>(std::get<PARAM_HAPTIC_SCALE_VIBRATOR_SCALE>(info.param)));
            std::string resonantFrequency = std::to_string(
                    std::get<PARAM_VIBRATION_INFORMATION_RESONANT_FREQUENCY>(info.param));
            std::string qFactor =
                    std::to_string(std::get<PARAM_VIBRATION_INFORMATION_Q_FACTOR>(info.param));
            std::string maxAmplitude =
                    std::to_string(std::get<PARAM_VIBRATION_INFORMATION_MAX_AMPLITUDE>(info.param));
            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               descriptor.common.id.uuid.toString() + "_hapticScaleId" +
                               hapticScaleID + "_hapticScaleVibScale" + hapticScaleVibScale +
                               "_resonantFrequency" + resonantFrequency + "_qFactor" + qFactor +
                               "_maxAmplitude" + maxAmplitude;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(HapticGeneratorParamTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
