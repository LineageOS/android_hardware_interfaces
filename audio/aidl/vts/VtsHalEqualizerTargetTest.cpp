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

#include <algorithm>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/audio/effect/IEffect.h>
#include <aidl/android/hardware/audio/effect/IFactory.h>
#define LOG_TAG "VtsHalEqualizerTest"
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android/binder_interface_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <gtest/gtest.h>

#include "AudioHalBinderServiceUtil.h"
#include "EffectHelper.h"
#include "TestUtils.h"

using namespace android;

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::Equalizer;
using aidl::android::hardware::audio::effect::getEffectTypeUuidEqualizer;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;

/**
 * Here we focus on specific effect (equalizer) parameter checking, general IEffect interfaces
 * testing performed in VtsAudioEfectTargetTest.
 */

enum ParamName { PARAM_INSTANCE_NAME, PARAM_PRESET, PARAM_BAND_LEVEL };
using EqualizerParamTestParam = std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int,
                                           std::vector<Equalizer::BandLevel>>;

/*
Testing parameter range, assuming the parameter supported by effect is in this range.
This range is verified with IEffect.getDescriptor(), for any index supported vts expect EX_NONE
from IEffect.setParameter(), otherwise expect EX_ILLEGAL_ARGUMENT.
*/
const std::vector<int> kBandLevels = {0, -10, 10};  // needs update with implementation

class EqualizerTest : public ::testing::TestWithParam<EqualizerParamTestParam>,
                      public EffectHelper {
  public:
    EqualizerTest()
        : mPresetIndex(std::get<PARAM_PRESET>(GetParam())),
          mBandLevel(std::get<PARAM_BAND_LEVEL>(GetParam())) {
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
    int mPresetIndex;
    std::vector<Equalizer::BandLevel> mBandLevel;

    void SetAndGetEqualizerParameters() {
        ASSERT_NE(nullptr, mEffect);
        for (auto& it : mTags) {
            auto& tag = it.first;
            auto& eq = it.second;

            // validate parameter
            const bool valid = isParameterValid<Equalizer, Range::equalizer>(eq, mDescriptor);
            const binder_exception_t expected = valid ? EX_NONE : EX_ILLEGAL_ARGUMENT;

            // set
            Parameter::Specific specific =
                    Parameter::Specific::make<Parameter::Specific::equalizer>(eq);
            Parameter expectParam = Parameter::make<Parameter::specific>(specific);
            EXPECT_STATUS(expected, mEffect->setParameter(expectParam))
                    << expectParam.toString() << "\n"
                    << mDescriptor.toString();

            // only get if parameter in range and set success
            if (expected == EX_NONE) {
                Parameter getParam;
                Equalizer::Id eqId = Equalizer::Id::make<Equalizer::Id::commonTag>(tag);
                Parameter::Id id = Parameter::Id::make<Parameter::Id::equalizerTag>(eqId);
                // if set success, then get should match
                EXPECT_STATUS(expected, mEffect->getParameter(id, &getParam));
                EXPECT_TRUE(isEqParameterExpected(expectParam, getParam))
                        << "\nexpect:" << expectParam.toString()
                        << "\ngetParam:" << getParam.toString();
            }
        }
    }

    bool isEqParameterExpected(const Parameter& expect, const Parameter& target) {
        // if parameter same, then for sure they are matched
        if (expect == target) return true;

        // if not, see if target include the expect parameter, and others all default (0).
        /*
         * This is to verify the case of client setParameter to a single bandLevel ({3, -1} for
         * example), and return of getParameter must be [{0, 0}, {1, 0}, {2, 0}, {3, -1}, {4, 0}]
         */
        EXPECT_EQ(expect.getTag(), Parameter::specific);
        EXPECT_EQ(target.getTag(), Parameter::specific);

        Parameter::Specific expectSpec = expect.get<Parameter::specific>(),
                            targetSpec = target.get<Parameter::specific>();
        EXPECT_EQ(expectSpec.getTag(), Parameter::Specific::equalizer);
        EXPECT_EQ(targetSpec.getTag(), Parameter::Specific::equalizer);

        Equalizer expectEq = expectSpec.get<Parameter::Specific::equalizer>(),
                  targetEq = targetSpec.get<Parameter::Specific::equalizer>();
        EXPECT_EQ(expectEq.getTag(), targetEq.getTag());

        auto eqTag = targetEq.getTag();
        switch (eqTag) {
            case Equalizer::bandLevels: {
                auto expectBl = expectEq.get<Equalizer::bandLevels>();
                std::sort(expectBl.begin(), expectBl.end(),
                          [](const auto& a, const auto& b) { return a.index < b.index; });
                expectBl.erase(std::unique(expectBl.begin(), expectBl.end()), expectBl.end());
                auto targetBl = targetEq.get<Equalizer::bandLevels>();
                return std::includes(targetBl.begin(), targetBl.end(), expectBl.begin(),
                                     expectBl.end());
            }
            case Equalizer::preset: {
                return expectEq.get<Equalizer::preset>() == targetEq.get<Equalizer::preset>();
            }
            default:
                return false;
        }
        return false;
    }

    void addPresetParam(int preset) {
        mTags.push_back({Equalizer::preset, Equalizer::make<Equalizer::preset>(preset)});
    }

    void addBandLevelsParam(std::vector<Equalizer::BandLevel>& bandLevels) {
        mTags.push_back(
                {Equalizer::bandLevels, Equalizer::make<Equalizer::bandLevels>(bandLevels)});
    }

  private:
    std::vector<std::pair<Equalizer::Tag, Equalizer>> mTags;

    void CleanUp() { mTags.clear(); }
};

TEST_P(EqualizerTest, SetAndGetParams) {
    addBandLevelsParam(mBandLevel);
    addPresetParam(mPresetIndex);
    EXPECT_NO_FATAL_FAILURE(SetAndGetEqualizerParameters());
}

std::vector<std::pair<std::shared_ptr<IFactory>, Descriptor>> kDescPair;
INSTANTIATE_TEST_SUITE_P(
        EqualizerTest, EqualizerTest,
        ::testing::Combine(
                testing::ValuesIn(kDescPair = EffectFactoryHelper::getAllEffectDescriptors(
                                          IFactory::descriptor, getEffectTypeUuidEqualizer())),
                testing::ValuesIn(EffectHelper::getTestValueSet<Equalizer, int, Range::equalizer,
                                                                Equalizer::preset>(
                        kDescPair, EffectHelper::expandTestValueBasic<int>)),
                testing::ValuesIn(
                        EffectHelper::getTestValueSet<Equalizer, std::vector<Equalizer::BandLevel>,
                                                      Range::equalizer, Equalizer::bandLevels>(
                                kDescPair,
                                [](std::set<std::vector<Equalizer::BandLevel>>& bandLevels) {
                                    return bandLevels;
                                }))),
        [](const testing::TestParamInfo<EqualizerTest::ParamType>& info) {
            auto descriptor = std::get<PARAM_INSTANCE_NAME>(info.param).second;
            std::string bandLevel =
                    ::android::internal::ToString(std::get<PARAM_BAND_LEVEL>(info.param));
            std::string name = getPrefix(descriptor) + "_preset_" +
                               std::to_string(std::get<PARAM_PRESET>(info.param)) + "_bandLevel_" +
                               bandLevel;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EqualizerTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
