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

#include <aidl/Vintf.h>
#define LOG_TAG "VtsHalHapticGeneratorTargetTest"
#include <android-base/logging.h>
#include <android/binder_enums.h>

#include "EffectHelper.h"

using namespace android;

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::getEffectTypeUuidHapticGenerator;
using aidl::android::hardware::audio::effect::HapticGenerator;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;
using android::hardware::audio::common::testing::detail::TestExecutionTracer;

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

        Parameter::Common common = EffectHelper::createParamCommon(
                0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */,
                kInputFrameCount /* iFrameCount */, kOutputFrameCount /* oFrameCount */);
        IEffect::OpenEffectReturn ret;
        ASSERT_NO_FATAL_FAILURE(open(mEffect, common, std::nullopt, &ret, EX_NONE));
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
    int mParamHapticScaleId = 0;
    HapticGenerator::VibratorScale mParamVibratorScale = HapticGenerator::VibratorScale::MUTE;
    float mParamResonantFrequency = 0;
    float mParamQFactor = 0;
    float mParamMaxAmplitude = 0;

    void SetAndGetHapticGeneratorParameters() {
        for (auto& it : mTags) {
            auto& tag = std::get<ParamTestEnum::PARAM_TEST_TAG>(it);
            auto& setHg = std::get<ParamTestEnum::PARAM_TEST_TARGET>(it);

            // set parameter
            Parameter expectParam;
            Parameter::Specific specific;
            specific.set<Parameter::Specific::hapticGenerator>(setHg);
            expectParam.set<Parameter::specific>(specific);
            EXPECT_STATUS(EX_NONE, mEffect->setParameter(expectParam)) << expectParam.toString();

            // get parameter
            Parameter getParam;
            Parameter::Id id;
            HapticGenerator::Id hgId;
            hgId.set<HapticGenerator::Id::commonTag>(tag);
            id.set<Parameter::Id::hapticGeneratorTag>(hgId);
            EXPECT_STATUS(EX_NONE, mEffect->getParameter(id, &getParam));
            EXPECT_EQ(expectParam, getParam) << expectParam.toString() << "\n"
                                             << getParam.toString();
        }
    }

    void addHapticScaleParam(int id, HapticGenerator::VibratorScale scale) {
        HapticGenerator setHg;
        std::vector<HapticGenerator::HapticScale> hapticScales = {{.id = id, .scale = scale}};
        setHg.set<HapticGenerator::hapticScales>(hapticScales);
        mTags.push_back({HapticGenerator::hapticScales, setHg});
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
    enum ParamTestEnum { PARAM_TEST_TAG, PARAM_TEST_TARGET };
    std::vector<std::tuple<HapticGenerator::Tag, HapticGenerator>> mTags;

    void CleanUp() { mTags.clear(); }
};

TEST_P(HapticGeneratorParamTest, SetAndGetHapticScale) {
    EXPECT_NO_FATAL_FAILURE(addHapticScaleParam(mParamHapticScaleId, mParamVibratorScale));
    SetAndGetHapticGeneratorParameters();
}

TEST_P(HapticGeneratorParamTest, SetAndGetMultipleHapticScales) {
    EXPECT_NO_FATAL_FAILURE(addHapticScaleParam(mParamHapticScaleId, mParamVibratorScale));
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
                                   IFactory::descriptor, getEffectTypeUuidHapticGenerator())),
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
            std::string name = getPrefix(descriptor) + "_hapticScaleId" + hapticScaleID +
                               "_hapticScaleVibScale" + hapticScaleVibScale + "_resonantFrequency" +
                               resonantFrequency + "_qFactor" + qFactor + "_maxAmplitude" +
                               maxAmplitude;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

INSTANTIATE_TEST_SUITE_P(
        HapticGeneratorInvalidTest, HapticGeneratorParamTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidHapticGenerator())),
                           testing::Values(MIN_ID),
                           testing::Values(HapticGenerator::VibratorScale::NONE),
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

// Test HapticScale[] hapticScales parameter
using HapticGeneratorScalesTestParam = std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>>;
class HapticGeneratorScalesTest : public ::testing::TestWithParam<HapticGeneratorScalesTestParam>,
                                  public EffectHelper {
  public:
    HapticGeneratorScalesTest() {
        std::tie(mFactory, mDescriptor) = std::get<PARAM_INSTANCE_NAME>(GetParam());
    }

    void SetUp() override {
        ASSERT_NE(nullptr, mFactory);
        ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));

        Parameter::Common common = EffectHelper::createParamCommon(
                0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */,
                kInputFrameCount /* iFrameCount */, kOutputFrameCount /* oFrameCount */);
        IEffect::OpenEffectReturn ret;
        ASSERT_NO_FATAL_FAILURE(open(mEffect, common, std::nullopt, &ret, EX_NONE));
        ASSERT_NE(nullptr, mEffect);
    }

    void TearDown() override {
        ASSERT_NO_FATAL_FAILURE(close(mEffect));
        ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
        CleanUp();
    }

    static const long kInputFrameCount = 0x100, kOutputFrameCount = 0x100;
    std::shared_ptr<IFactory> mFactory;
    std::shared_ptr<IEffect> mEffect;
    Descriptor mDescriptor;

    void addHapticScaleParam(std::vector<HapticGenerator::HapticScale> scales) {
        mHapticScales.push_back(HapticGenerator::make<HapticGenerator::hapticScales>(scales));
        for (const auto& scale : scales) {
            expectMap.insert_or_assign(scale.id, scale.scale);
        }
    }

    void SetHapticScaleParameters() {
        // std::unordered_set<HapticGenerator::HapticScale> target;
        for (auto& it : mHapticScales) {
            Parameter::Specific specific =
                    Parameter::Specific::make<Parameter::Specific::hapticGenerator>(it);
            Parameter param = Parameter::make<Parameter::specific>(specific);
            EXPECT_STATUS(EX_NONE, mEffect->setParameter(param)) << param.toString();
        }
    }

    void checkHapticScaleParameter() {
        // get parameter
        Parameter targetParam;
        HapticGenerator::Id hgId = HapticGenerator::Id::make<HapticGenerator::Id::commonTag>(
                HapticGenerator::hapticScales);
        Parameter::Id id = Parameter::Id::make<Parameter::Id::hapticGeneratorTag>(hgId);
        EXPECT_STATUS(EX_NONE, mEffect->getParameter(id, &targetParam));
        ASSERT_EQ(Parameter::specific, targetParam.getTag());
        Parameter::Specific specific = targetParam.get<Parameter::specific>();
        ASSERT_EQ(Parameter::Specific::hapticGenerator, specific.getTag());
        HapticGenerator hg = specific.get<Parameter::Specific::hapticGenerator>();
        ASSERT_EQ(HapticGenerator::hapticScales, hg.getTag());
        std::vector<HapticGenerator::HapticScale> scales = hg.get<HapticGenerator::hapticScales>();
        ASSERT_EQ(scales.size(), expectMap.size());
        for (const auto& scale : scales) {
            auto itor = expectMap.find(scale.id);
            ASSERT_NE(expectMap.end(), itor);
            ASSERT_EQ(scale.scale, itor->second);
            expectMap.erase(scale.id);
        }
        ASSERT_EQ(0ul, expectMap.size());
    }

    const static HapticGenerator::HapticScale kHapticScaleWithMinId;
    const static HapticGenerator::HapticScale kHapticScaleWithMinIdNew;
    const static HapticGenerator::HapticScale kHapticScale;
    const static HapticGenerator::HapticScale kHapticScaleNew;
    const static HapticGenerator::HapticScale kHapticScaleWithMaxId;
    const static HapticGenerator::HapticScale kHapticScaleWithMaxIdNew;

    std::vector<HapticGenerator> mHapticScales;

    void CleanUp() {
        mHapticScales.clear();
        expectMap.clear();
    }

  private:
    std::map<int /* trackID */, HapticGenerator::VibratorScale> expectMap;
};

const HapticGenerator::HapticScale HapticGeneratorScalesTest::kHapticScaleWithMinId = {
        .id = MIN_ID, .scale = HapticGenerator::VibratorScale::MUTE};
const HapticGenerator::HapticScale HapticGeneratorScalesTest::kHapticScaleWithMinIdNew = {
        .id = MIN_ID, .scale = HapticGenerator::VibratorScale::VERY_LOW};
const HapticGenerator::HapticScale HapticGeneratorScalesTest::kHapticScale = {
        .id = 1, .scale = HapticGenerator::VibratorScale::LOW};
const HapticGenerator::HapticScale HapticGeneratorScalesTest::kHapticScaleNew = {
        .id = 1, .scale = HapticGenerator::VibratorScale::NONE};
const HapticGenerator::HapticScale HapticGeneratorScalesTest::kHapticScaleWithMaxId = {
        .id = MAX_ID, .scale = HapticGenerator::VibratorScale::VERY_HIGH};
const HapticGenerator::HapticScale HapticGeneratorScalesTest::kHapticScaleWithMaxIdNew = {
        .id = MAX_ID, .scale = HapticGenerator::VibratorScale::MUTE};

TEST_P(HapticGeneratorScalesTest, SetAndUpdateOne) {
    EXPECT_NO_FATAL_FAILURE(addHapticScaleParam({kHapticScale}));
    EXPECT_NO_FATAL_FAILURE(SetHapticScaleParameters());
    EXPECT_NO_FATAL_FAILURE(addHapticScaleParam({kHapticScaleNew}));
    EXPECT_NO_FATAL_FAILURE(SetHapticScaleParameters());

    EXPECT_NO_FATAL_FAILURE(addHapticScaleParam({kHapticScaleWithMinId}));
    EXPECT_NO_FATAL_FAILURE(SetHapticScaleParameters());
    EXPECT_NO_FATAL_FAILURE(addHapticScaleParam({kHapticScaleWithMinIdNew}));
    EXPECT_NO_FATAL_FAILURE(SetHapticScaleParameters());

    EXPECT_NO_FATAL_FAILURE(addHapticScaleParam({kHapticScaleWithMaxId}));
    EXPECT_NO_FATAL_FAILURE(SetHapticScaleParameters());
    EXPECT_NO_FATAL_FAILURE(addHapticScaleParam({kHapticScaleWithMaxIdNew}));
    EXPECT_NO_FATAL_FAILURE(SetHapticScaleParameters());

    EXPECT_NO_FATAL_FAILURE(checkHapticScaleParameter());
}

TEST_P(HapticGeneratorScalesTest, SetAndUpdateVector) {
    EXPECT_NO_FATAL_FAILURE(
            addHapticScaleParam({kHapticScale, kHapticScaleWithMaxId, kHapticScaleWithMinId}));
    EXPECT_NO_FATAL_FAILURE(SetHapticScaleParameters());
    EXPECT_NO_FATAL_FAILURE(addHapticScaleParam(
            {kHapticScaleNew, kHapticScaleWithMaxIdNew, kHapticScaleWithMinIdNew}));
    EXPECT_NO_FATAL_FAILURE(SetHapticScaleParameters());

    EXPECT_NO_FATAL_FAILURE(checkHapticScaleParameter());
}

TEST_P(HapticGeneratorScalesTest, SetAndUpdateMultipleVector) {
    EXPECT_NO_FATAL_FAILURE(
            addHapticScaleParam({kHapticScale, kHapticScaleWithMaxId, kHapticScaleWithMinId}));
    EXPECT_NO_FATAL_FAILURE(SetHapticScaleParameters());
    EXPECT_NO_FATAL_FAILURE(addHapticScaleParam(
            {kHapticScaleNew, kHapticScaleWithMaxIdNew, kHapticScaleWithMinIdNew}));
    EXPECT_NO_FATAL_FAILURE(SetHapticScaleParameters());
    EXPECT_NO_FATAL_FAILURE(
            addHapticScaleParam({kHapticScale, kHapticScaleWithMaxId, kHapticScaleWithMinId}));
    EXPECT_NO_FATAL_FAILURE(SetHapticScaleParameters());

    EXPECT_NO_FATAL_FAILURE(checkHapticScaleParameter());
}

TEST_P(HapticGeneratorScalesTest, SetOneAndAddMoreVector) {
    EXPECT_NO_FATAL_FAILURE(addHapticScaleParam({kHapticScale}));
    EXPECT_NO_FATAL_FAILURE(SetHapticScaleParameters());
    EXPECT_NO_FATAL_FAILURE(addHapticScaleParam({kHapticScaleWithMaxId, kHapticScaleWithMinId}));
    EXPECT_NO_FATAL_FAILURE(SetHapticScaleParameters());

    EXPECT_NO_FATAL_FAILURE(checkHapticScaleParameter());
}

TEST_P(HapticGeneratorScalesTest, SetMultipleAndAddOneVector) {
    EXPECT_NO_FATAL_FAILURE(addHapticScaleParam({kHapticScaleWithMaxId, kHapticScaleWithMinId}));
    EXPECT_NO_FATAL_FAILURE(SetHapticScaleParameters());
    EXPECT_NO_FATAL_FAILURE(addHapticScaleParam({kHapticScale}));
    EXPECT_NO_FATAL_FAILURE(SetHapticScaleParameters());

    EXPECT_NO_FATAL_FAILURE(checkHapticScaleParameter());
}

TEST_P(HapticGeneratorScalesTest, SetMultipleVectorRepeat) {
    EXPECT_NO_FATAL_FAILURE(
            addHapticScaleParam({kHapticScaleWithMaxId, kHapticScale, kHapticScaleWithMinId}));
    EXPECT_NO_FATAL_FAILURE(SetHapticScaleParameters());
    EXPECT_NO_FATAL_FAILURE(
            addHapticScaleParam({kHapticScaleWithMaxId, kHapticScale, kHapticScaleWithMinId}));
    EXPECT_NO_FATAL_FAILURE(SetHapticScaleParameters());
    EXPECT_NO_FATAL_FAILURE(
            addHapticScaleParam({kHapticScaleWithMaxId, kHapticScale, kHapticScaleWithMinId}));
    EXPECT_NO_FATAL_FAILURE(SetHapticScaleParameters());

    EXPECT_NO_FATAL_FAILURE(checkHapticScaleParameter());
}

INSTANTIATE_TEST_SUITE_P(
        HapticGeneratorScalesTest, HapticGeneratorScalesTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                IFactory::descriptor, getEffectTypeUuidHapticGenerator()))),
        [](const testing::TestParamInfo<HapticGeneratorScalesTest::ParamType>& info) {
            auto descriptor = std::get<PARAM_INSTANCE_NAME>(info.param).second;
            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               descriptor.common.id.uuid.toString();
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(HapticGeneratorScalesTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::UnitTest::GetInstance()->listeners().Append(new TestExecutionTracer());
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
