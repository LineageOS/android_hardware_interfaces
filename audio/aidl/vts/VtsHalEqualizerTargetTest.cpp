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
#include <string>
#include <vector>

#define LOG_TAG "VtsHalEqualizerTest"
#include <aidl/Gtest.h>
#include <aidl/android/hardware/audio/effect/IEffect.h>
#include <aidl/android/hardware/audio/effect/IFactory.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android/binder_interface_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <gtest/gtest.h>

#include "EffectHelper.h"
#include "TestUtils.h"

using namespace android;

using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::Equalizer;
using aidl::android::hardware::audio::effect::getEffectTypeUuidEqualizer;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;
using android::hardware::audio::common::testing::detail::TestExecutionTracer;

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

class EqualizerTestHelper : public EffectHelper {
  public:
    EqualizerTestHelper(std::pair<std::shared_ptr<IFactory>, Descriptor> descPair,
                        int presetIndex = 0,
                        std::vector<Equalizer::BandLevel> bandLevel =
                                std::vector<Equalizer::BandLevel>{
                                        Equalizer::BandLevel({.index = 0, .levelMb = 0})})
        : mFactory(descPair.first), mPresetIndex(presetIndex), mBandLevel(bandLevel) {
        mDescriptor = descPair.second;
    }

    void SetUpEqualizer() {
        ASSERT_NE(nullptr, mFactory);
        ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));

        AudioChannelLayout inputLayout = AudioChannelLayout::make<AudioChannelLayout::layoutMask>(
                AudioChannelLayout::LAYOUT_MONO);
        AudioChannelLayout outputLayout = inputLayout;

        Parameter::Common common = createParamCommon(
                0 /* session */, 1 /* ioHandle */, kSamplingFrequency /* iSampleRate */,
                kSamplingFrequency /* oSampleRate */, kInputFrameCount /* iFrameCount */,
                kOutputFrameCount /* oFrameCount */, inputLayout, outputLayout);
        ASSERT_NO_FATAL_FAILURE(open(mEffect, common, std::nullopt, &mOpenEffectReturn, EX_NONE));
        ASSERT_NE(nullptr, mEffect);
        mVersion = EffectFactoryHelper::getHalVersion(mFactory);
    }

    void TearDownEqualizer() {
        ASSERT_NO_FATAL_FAILURE(close(mEffect));
        ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
        mOpenEffectReturn = IEffect::OpenEffectReturn{};
    }

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

    void addBandLevelsParam(const std::vector<Equalizer::BandLevel>& bandLevels) {
        mTags.push_back(
                {Equalizer::bandLevels, Equalizer::make<Equalizer::bandLevels>(bandLevels)});
    }

    static const long kInputFrameCount = 0x10000, kOutputFrameCount = 0x10000;
    const std::shared_ptr<IFactory> mFactory;
    const int mPresetIndex;
    const std::vector<Equalizer::BandLevel> mBandLevel;
    int mVersion = 0;
    std::shared_ptr<IEffect> mEffect;
    IEffect::OpenEffectReturn mOpenEffectReturn;

  private:
    std::vector<std::pair<Equalizer::Tag, Equalizer>> mTags;
    void CleanUp() { mTags.clear(); }
};

class EqualizerParamTest : public ::testing::TestWithParam<EqualizerParamTestParam>,
                           public EqualizerTestHelper {
  public:
    EqualizerParamTest()
        : EqualizerTestHelper(std::get<PARAM_INSTANCE_NAME>(GetParam()),
                              std::get<PARAM_PRESET>(GetParam()),
                              std::get<PARAM_BAND_LEVEL>(GetParam())) {}

    void SetUp() override { ASSERT_NO_FATAL_FAILURE(SetUpEqualizer()); }

    void TearDown() override { ASSERT_NO_FATAL_FAILURE(TearDownEqualizer()); }
};

TEST_P(EqualizerParamTest, SetAndGetParams) {
    addBandLevelsParam(mBandLevel);
    addPresetParam(mPresetIndex);
    ASSERT_NO_FATAL_FAILURE(SetAndGetEqualizerParameters());
}

using EqualizerDataTestParam = std::pair<std::shared_ptr<IFactory>, Descriptor>;

class EqualizerDataTest : public ::testing::TestWithParam<EqualizerDataTestParam>,
                          public EqualizerTestHelper {
  public:
    EqualizerDataTest()
        : EqualizerTestHelper(GetParam()),
          mInputBuffer(kInputFrameCount),
          mOutputBuffer(kOutputFrameCount) {}

    template <Equalizer::Tag TagValue>
    auto getEqualizerParam() {
        Parameter getParam;
        Equalizer::Id eqId = Equalizer::Id::make<Equalizer::Id::commonTag>(TagValue);
        Parameter::Id id = Parameter::Id::make<Parameter::Id::equalizerTag>(eqId);
        EXPECT_STATUS(EX_NONE, mEffect->getParameter(id, &getParam));
        return getParam.get<Parameter::specific>()
                .get<Parameter::Specific::equalizer>()
                .get<TagValue>();  // Attempting to use the Tag type
    }

    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(SetUpEqualizer());
        SKIP_TEST_IF_DATA_UNSUPPORTED(mDescriptor.common.flags);
        mBandLevels = getEqualizerParam<Equalizer::bandLevels>();

        auto centerFrequencies = getEqualizerParam<Equalizer::centerFreqMh>();
        ASSERT_EQ(centerFrequencies.size(), mBandLevels.size());
        // convert center frequencies into Hz unit
        for (auto& freq : centerFrequencies) {
            freq = freq / 1000;
        }

        mBinOffsets.resize(centerFrequencies.size());
        mOutputMag.resize(mBinOffsets.size());

        roundToFreqCenteredToFftBin(centerFrequencies, mBinOffsets, kBinWidth);

        ASSERT_NO_FATAL_FAILURE(generateSineWave(centerFrequencies, mInputBuffer, 1.0,
                                                 kSamplingFrequency,
                                                 AudioChannelLayout::LAYOUT_MONO));
    }

    void TearDown() override { ASSERT_NO_FATAL_FAILURE(TearDownEqualizer()); }

    static constexpr float kBinWidth = (float)kSamplingFrequency / kNPointFFT;
    std::vector<float> mInputBuffer;
    std::vector<float> mOutputBuffer;
    std::vector<Equalizer::BandLevel> mBandLevels;
    std::vector<int> mBinOffsets;
    std::vector<float> mOutputMag;
};

TEST_P(EqualizerDataTest, testBandLevels) {
    auto bandFrequencies = getEqualizerParam<Equalizer::bandFrequencies>();
    ASSERT_EQ(bandFrequencies.size(), mBandLevels.size());

    std::vector<Equalizer::BandLevel> testBandLevelMb(mBandLevels.size());
    for (size_t i = 0; i < testBandLevelMb.size(); i++) {
        testBandLevelMb[i] = {static_cast<int>(i), 0};
    }

    constexpr float kScalingFactor = 3.0;
    std::vector<int> testlevelMbValues = {-1500, -1000, -500, 500, 1000, 1500};
    size_t centerBandIndex = mBandLevels.size() / 2;

    for (int levelMb : testlevelMbValues) {
        for (size_t i = 0; i < mBandLevels.size(); i++) {
            // set bandLevel
            testBandLevelMb[i] = {static_cast<int>(i), levelMb};
            Parameter::Specific specific =
                    Parameter::Specific::make<Parameter::Specific::equalizer>(
                            Equalizer::make<Equalizer::bandLevels>(testBandLevelMb));
            Parameter expectParam = Parameter::make<Parameter::specific>(specific);
            EXPECT_STATUS(EX_NONE, mEffect->setParameter(expectParam))
                    << expectParam.toString() << "\n"
                    << mDescriptor.toString();

            ASSERT_NO_FATAL_FAILURE(processAndWriteToOutput(mInputBuffer, mOutputBuffer, mEffect,
                                                            mOpenEffectReturn, mVersion));

            EXPECT_NO_FATAL_FAILURE(
                    calculateMagnitudeMono(mOutputMag, mOutputBuffer, mBinOffsets, kNPointFFT));

            size_t referenceBandIndex = (i == centerBandIndex) ? 0 : centerBandIndex;

            if (levelMb > 0) {
                EXPECT_GE(mOutputMag[i] - mOutputMag[referenceBandIndex], levelMb)
                        << "Output magnitude difference from reference band should be greater than "
                           "or equal to set levelMb value ("
                        << levelMb << " mB)";
            } else {
                EXPECT_LT(mOutputMag[i] - mOutputMag[referenceBandIndex],
                          (float)levelMb / kScalingFactor)
                        << "Output magnitude difference from reference band should be lesser than "
                           "set levelMb value / scaling factor ("
                        << levelMb << " mB / " << kScalingFactor << ") in case of negative gain";
            }
            testBandLevelMb[i] = {static_cast<int>(i), 0};
        }
    }
}

TEST_P(EqualizerDataTest, testPresets) {
    constexpr float kToleranceDb = 1.0;
    constexpr int kCustomPresetIndex = -1;

    auto presets = getEqualizerParam<Equalizer::presets>();

    std::vector<float> inputMag(mBinOffsets.size());
    EXPECT_NO_FATAL_FAILURE(
            calculateMagnitudeMono(inputMag, mInputBuffer, mBinOffsets, kNPointFFT));

    for (auto preset : presets) {
        // Skip for 'Custom' preset value as it is currently not supported
        if (preset.index == kCustomPresetIndex) {
            continue;
        }
        // set preset
        Parameter::Specific specific = Parameter::Specific::make<Parameter::Specific::equalizer>(
                Equalizer::make<Equalizer::preset>(static_cast<int>(preset.index)));
        Parameter expectParam = Parameter::make<Parameter::specific>(specific);
        EXPECT_STATUS(EX_NONE, mEffect->setParameter(expectParam)) << expectParam.toString() << "\n"
                                                                   << mDescriptor.toString();

        ASSERT_NO_FATAL_FAILURE(processAndWriteToOutput(mInputBuffer, mOutputBuffer, mEffect,
                                                        mOpenEffectReturn, mVersion));

        EXPECT_NO_FATAL_FAILURE(
                calculateMagnitudeMono(mOutputMag, mOutputBuffer, mBinOffsets, kNPointFFT));

        // get band levels
        mBandLevels = getEqualizerParam<Equalizer::bandLevels>();

        for (size_t i = 1; i < mBandLevels.size(); i++) {
            int expectedAdjacentBandLevelMbDiff =
                    (mBandLevels[i].levelMb - mBandLevels[i - 1].levelMb);

            ASSERT_NE(inputMag[i], 0);
            if (i == 1) {
                ASSERT_NE(inputMag[i - 1], 0);
            }
            float actualAdjacentBandGainDbDiff = 20 * (log10(mOutputMag[i] / inputMag[i]) -
                                                       log10(mOutputMag[i - 1] / inputMag[i - 1]));

            if (expectedAdjacentBandLevelMbDiff == 0) {
                EXPECT_LT(abs(actualAdjacentBandGainDbDiff), kToleranceDb)
                        << "For eq preset : " << preset.name << "(" << preset.index << ")"
                        << ", between bands " << i << " and " << i - 1
                        << ", expected relative gain is less than kToleranceDb, got relative gain "
                           ": "
                        << actualAdjacentBandGainDbDiff;
            } else {
                EXPECT_GT(expectedAdjacentBandLevelMbDiff * actualAdjacentBandGainDbDiff, 0)
                        << "For eq preset : " << preset.name << "(" << preset.index << ")"
                        << ", between bands " << i << " and " << i - 1
                        << ", expected relative gain and seen relative magnitude difference are of "
                           "opposite signs. Expected relative gain : "
                        << expectedAdjacentBandLevelMbDiff
                        << ", seen magnitude difference : " << actualAdjacentBandGainDbDiff;
            }
        }
    }
}

std::vector<std::pair<std::shared_ptr<IFactory>, Descriptor>> kDescPair;
INSTANTIATE_TEST_SUITE_P(
        EqualizerTest, EqualizerParamTest,
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
        [](const testing::TestParamInfo<EqualizerParamTest::ParamType>& info) {
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
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EqualizerParamTest);

INSTANTIATE_TEST_SUITE_P(EqualizerTest, EqualizerDataTest,
                         testing::ValuesIn(kDescPair = EffectFactoryHelper::getAllEffectDescriptors(
                                                   IFactory::descriptor,
                                                   getEffectTypeUuidEqualizer())),
                         [](const testing::TestParamInfo<EqualizerDataTest::ParamType>& info) {
                             auto descriptor = (info.param).second;
                             std::string name = getPrefix(descriptor);
                             std::replace_if(
                                     name.begin(), name.end(),
                                     [](const char c) { return !std::isalnum(c); }, '_');
                             return name;
                         });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EqualizerDataTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::UnitTest::GetInstance()->listeners().Append(new TestExecutionTracer());
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
