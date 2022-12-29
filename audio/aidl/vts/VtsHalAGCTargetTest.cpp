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

#include <Utils.h>
#include <aidl/Vintf.h>
#include <android/binder_enums.h>
#include <unordered_set>

#define LOG_TAG "VtsHalAGCParamTest"

#include "EffectHelper.h"

using namespace android;

using aidl::android::hardware::audio::effect::AutomaticGainControl;
using aidl::android::hardware::audio::effect::Capability;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::kAutomaticGainControlTypeUUID;
using aidl::android::hardware::audio::effect::Parameter;

enum ParamName {
    PARAM_INSTANCE_NAME,
    PARAM_DIGITAL_GAIN,
    PARAM_SATURATION_MARGIN,
    PARAM_LEVEL_ESTIMATOR
};
using AGCParamTestParam =
        std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int /* gain */,
                   int /* margin */, AutomaticGainControl::LevelEstimator>;

class AGCParamTest : public ::testing::TestWithParam<AGCParamTestParam>, public EffectHelper {
  public:
    AGCParamTest()
        : mGain(std::get<PARAM_DIGITAL_GAIN>(GetParam())),
          mMargin(std::get<PARAM_SATURATION_MARGIN>(GetParam())),
          mLevelEstimator(std::get<PARAM_LEVEL_ESTIMATOR>(GetParam())) {
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
        AutomaticGainControl AGC =
                AutomaticGainControl::make<AutomaticGainControl::fixedDigitalGainMb>(0);
        Parameter::Specific specific =
                Parameter::Specific::make<Parameter::Specific::automaticGainControl>(AGC);
        return specific;
    }

    static const long kInputFrameCount = 0x100, kOutputFrameCount = 0x100;
    std::shared_ptr<IFactory> mFactory;
    std::shared_ptr<IEffect> mEffect;
    Descriptor mDescriptor;
    int mGain;
    int mMargin;
    AutomaticGainControl::LevelEstimator mLevelEstimator;

    void SetAndGetParameters() {
        for (auto& it : mTags) {
            auto& tag = it.first;
            auto& AGC = it.second;

            // validate parameter
            Descriptor desc;
            ASSERT_STATUS(EX_NONE, mEffect->getDescriptor(&desc));
            const bool valid = isTagInRange(tag, AGC, desc);
            const binder_exception_t expected = valid ? EX_NONE : EX_ILLEGAL_ARGUMENT;

            // set parameter
            Parameter expectParam;
            Parameter::Specific specific;
            specific.set<Parameter::Specific::automaticGainControl>(AGC);
            expectParam.set<Parameter::specific>(specific);
            EXPECT_STATUS(expected, mEffect->setParameter(expectParam)) << expectParam.toString();

            // only get if parameter in range and set success
            if (expected == EX_NONE) {
                Parameter getParam;
                Parameter::Id id;
                AutomaticGainControl::Id specificId;
                specificId.set<AutomaticGainControl::Id::commonTag>(tag);
                id.set<Parameter::Id::automaticGainControlTag>(specificId);
                EXPECT_STATUS(EX_NONE, mEffect->getParameter(id, &getParam));

                EXPECT_EQ(expectParam, getParam) << "\nexpect:" << expectParam.toString()
                                                 << "\ngetParam:" << getParam.toString();
            }
        }
    }

    void addDigitalGainParam(int gain) {
        AutomaticGainControl AGC;
        AGC.set<AutomaticGainControl::fixedDigitalGainMb>(gain);
        mTags.push_back({AutomaticGainControl::fixedDigitalGainMb, AGC});
    }
    void addSaturationMarginParam(int margin) {
        AutomaticGainControl AGC;
        AGC.set<AutomaticGainControl::saturationMarginMb>(margin);
        mTags.push_back({AutomaticGainControl::saturationMarginMb, AGC});
    }
    void addLevelEstimatorParam(AutomaticGainControl::LevelEstimator levelEstimator) {
        AutomaticGainControl AGC;
        AGC.set<AutomaticGainControl::levelEstimator>(levelEstimator);
        mTags.push_back({AutomaticGainControl::levelEstimator, AGC});
    }

    bool isTagInRange(const AutomaticGainControl::Tag& tag, const AutomaticGainControl& AGC,
                      const Descriptor& desc) const {
        const AutomaticGainControl::Capability& AGCCap =
                desc.capability.get<Capability::automaticGainControl>();
        switch (tag) {
            case AutomaticGainControl::fixedDigitalGainMb: {
                auto gain = AGC.get<AutomaticGainControl::fixedDigitalGainMb>();
                return gain >= 0 && gain <= AGCCap.maxFixedDigitalGainMb;
            }
            case AutomaticGainControl::levelEstimator: {
                return true;
            }
            case AutomaticGainControl::saturationMarginMb: {
                auto margin = AGC.get<AutomaticGainControl::saturationMarginMb>();
                return margin >= 0 && margin <= AGCCap.maxSaturationMarginMb;
            }
            default:
                return false;
        }
    }
    static std::unordered_set<int> getDigitalGainValues() {
        auto descList = EffectFactoryHelper::getAllEffectDescriptors(IFactory::descriptor,
                                                                     kAutomaticGainControlTypeUUID);
        const auto max = std::max_element(
                descList.begin(), descList.end(),
                [](const std::pair<std::shared_ptr<IFactory>, Descriptor>& a,
                   const std::pair<std::shared_ptr<IFactory>, Descriptor>& b) {
                    return a.second.capability.get<Capability::automaticGainControl>()
                                   .maxFixedDigitalGainMb <
                           b.second.capability.get<Capability::automaticGainControl>()
                                   .maxFixedDigitalGainMb;
                });
        if (max == descList.end()) {
            return {0};
        }
        int maxGain = max->second.capability.get<Capability::automaticGainControl>()
                              .maxFixedDigitalGainMb;
        return {-1, 0, maxGain - 1, maxGain, maxGain + 1};
    }
    static std::unordered_set<int> getSaturationMarginValues() {
        auto descList = EffectFactoryHelper::getAllEffectDescriptors(IFactory::descriptor,
                                                                     kAutomaticGainControlTypeUUID);
        const auto max = std::max_element(
                descList.begin(), descList.end(),
                [](const std::pair<std::shared_ptr<IFactory>, Descriptor>& a,
                   const std::pair<std::shared_ptr<IFactory>, Descriptor>& b) {
                    return a.second.capability.get<Capability::automaticGainControl>()
                                   .maxSaturationMarginMb <
                           b.second.capability.get<Capability::automaticGainControl>()
                                   .maxSaturationMarginMb;
                });
        if (max == descList.end()) {
            return {0};
        }
        int maxMargin = max->second.capability.get<Capability::automaticGainControl>()
                                .maxSaturationMarginMb;
        return {-1, 0, maxMargin - 1, maxMargin, maxMargin + 1};
    }
    static std::unordered_set<AutomaticGainControl::LevelEstimator> getLevelEstimatorValues() {
        return {ndk::enum_range<AutomaticGainControl::LevelEstimator>().begin(),
                ndk::enum_range<AutomaticGainControl::LevelEstimator>().end()};
    }

  private:
    std::vector<std::pair<AutomaticGainControl::Tag, AutomaticGainControl>> mTags;
    void CleanUp() { mTags.clear(); }
};

TEST_P(AGCParamTest, SetAndGetDigitalGainParam) {
    EXPECT_NO_FATAL_FAILURE(addDigitalGainParam(mGain));
    SetAndGetParameters();
}

TEST_P(AGCParamTest, SetAndGetSaturationMargin) {
    EXPECT_NO_FATAL_FAILURE(addSaturationMarginParam(mMargin));
    SetAndGetParameters();
}

TEST_P(AGCParamTest, SetAndGetLevelEstimator) {
    EXPECT_NO_FATAL_FAILURE(addLevelEstimatorParam(mLevelEstimator));
    SetAndGetParameters();
}

INSTANTIATE_TEST_SUITE_P(
        AGCParamTest, AGCParamTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, kAutomaticGainControlTypeUUID)),
                           testing::ValuesIn(AGCParamTest::getDigitalGainValues()),
                           testing::ValuesIn(AGCParamTest::getSaturationMarginValues()),
                           testing::ValuesIn(AGCParamTest::getLevelEstimatorValues())),
        [](const testing::TestParamInfo<AGCParamTest::ParamType>& info) {
            auto descriptor = std::get<PARAM_INSTANCE_NAME>(info.param).second;
            std::string gain = std::to_string(std::get<PARAM_DIGITAL_GAIN>(info.param));
            std::string estimator = aidl::android::hardware::audio::effect::toString(
                    std::get<PARAM_LEVEL_ESTIMATOR>(info.param));
            std::string margin =
                    std::to_string(static_cast<int>(std::get<PARAM_SATURATION_MARGIN>(info.param)));

            std::string name = "Implementor_" + descriptor.common.implementor + "_name_" +
                               descriptor.common.name + "_UUID_" +
                               descriptor.common.id.uuid.toString() + "_digital_gain_" + gain +
                               "_level_estimator_" + estimator + "_margin_" + margin;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AGCParamTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}