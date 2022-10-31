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
#include <string>
#include <vector>

#define LOG_TAG "VtsHalEqualizerTest"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android/binder_interface_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <gtest/gtest.h>

#include <Utils.h>
#include <aidl/android/hardware/audio/effect/IEffect.h>
#include <aidl/android/hardware/audio/effect/IFactory.h>
#include <aidl/android/media/audio/common/AudioChannelLayout.h>
#include <aidl/android/media/audio/common/AudioDeviceType.h>

#include "AudioHalBinderServiceUtil.h"
#include "EffectHelper.h"
#include "TestUtils.h"
#include "effect-impl/EffectUUID.h"

using namespace android;

using aidl::android::hardware::audio::effect::Capability;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::EffectNullUuid;
using aidl::android::hardware::audio::effect::Equalizer;
using aidl::android::hardware::audio::effect::EqualizerTypeUUID;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;

/**
 * Here we focus on specific parameter checking, general IEffect interfaces testing performed in
 * VtsAudioEfectTargetTest.
 */
enum ParamName { PARAM_INSTANCE_NAME, PARAM_PRESET_INDEX, PARAM_BAND_INDEX, PARAM_BAND_LEVEL };
using EqualizerParamTestParam = std::tuple<std::string, int, int, int>;

/*
Testing parameter range, assuming the parameter supported by effect is in this range.
This range is verified with IEffect.getDescriptor(), for any index supported vts expect EX_NONE
from IEffect.setParameter(), otherwise expect EX_ILLEGAL_ARGUMENT.
*/
constexpr std::pair<int, int> kPresetIndexRange = {-1, 10};  // valid range [0, 9]
constexpr std::pair<int, int> kBandIndexRange = {-1, 5};     // valid range [0, 4]
constexpr std::pair<int, int> kBandLevelRange = {-5, 5};     // needs update with implementation

class EqualizerParamTest : public ::testing::TestWithParam<EqualizerParamTestParam>,
                           public EffectHelper {
  public:
    EqualizerParamTest()
        : EffectHelper(std::get<PARAM_INSTANCE_NAME>(GetParam())),
          mParamPresetIndex(std::get<PARAM_PRESET_INDEX>(GetParam())),
          mParamBandIndex(std::get<PARAM_BAND_INDEX>(GetParam())),
          mParamBandLevel(std::get<PARAM_BAND_LEVEL>(GetParam())) {}

    void SetUp() override {
        CreateEffectsWithUUID(EqualizerTypeUUID);
        initParamCommonFormat();
        initParamCommon();
        initParamSpecific();
        OpenEffects(EqualizerTypeUUID);
        SCOPED_TRACE(testing::Message() << "preset: " << mParamPresetIndex << " bandIdx "
                                        << mParamBandIndex << " level " << mParamBandLevel);
    }

    void TearDown() override {
        CloseEffects();
        DestroyEffects();
        CleanUp();
    }

    const int mParamPresetIndex;
    const int mParamBandIndex;
    const int mParamBandLevel;

    void SetAndGetEqualizerParameters() {
        auto functor = [&](const std::shared_ptr<IEffect>& effect) {
            for (auto& it : mTags) {
                auto& tag = it.first;
                auto& eq = it.second;

                // validate parameter
                Descriptor desc;
                ASSERT_STATUS(EX_NONE, effect->getDescriptor(&desc));
                const bool valid = isTagInRange(it.first, it.second, desc);
                const binder_exception_t expected = valid ? EX_NONE : EX_ILLEGAL_ARGUMENT;

                // set
                Parameter expectParam;
                Parameter::Specific specific;
                specific.set<Parameter::Specific::equalizer>(*eq.get());
                expectParam.set<Parameter::specific>(specific);
                EXPECT_STATUS(expected, effect->setParameter(expectParam))
                        << expectParam.toString();

                // only get if parameter in range and set success
                if (expected == EX_NONE) {
                    Parameter getParam;
                    Parameter::Specific::Id id;
                    id.set<Parameter::Specific::Id::equalizerTag>(tag);
                    // if set success, then get should match
                    EXPECT_STATUS(expected, effect->getParameter(id, &getParam));
                    EXPECT_EQ(expectParam, getParam) << "\n"
                                                     << expectParam.toString() << "\n"
                                                     << getParam.toString();
                }
            }
        };
        EXPECT_NO_FATAL_FAILURE(ForEachEffect(functor));
    }

    void addPresetParam(int preset) {
        Equalizer eq;
        eq.set<Equalizer::preset>(preset);
        mTags.push_back({Equalizer::preset, std::make_unique<Equalizer>(std::move(eq))});
    }

    void addBandLevelsParam(std::vector<Equalizer::BandLevel>& bandLevels) {
        Equalizer eq;
        eq.set<Equalizer::bandLevels>(bandLevels);
        mTags.push_back({Equalizer::bandLevels, std::make_unique<Equalizer>(std::move(eq))});
    }

    bool isTagInRange(const Equalizer::Tag& tag, const std::unique_ptr<Equalizer>& eq,
                      const Descriptor& desc) const {
        std::cout << "xxx" << toString(tag) << " " << desc.toString();
        const Equalizer::Capability& eqCap = desc.capability.get<Capability::equalizer>();
        switch (tag) {
            case Equalizer::preset: {
                int index = eq->get<Equalizer::preset>();
                return isPresetIndexInRange(eqCap, index);
            }
            case Equalizer::bandLevels: {
                auto& bandLevel = eq->get<Equalizer::bandLevels>();
                return isBandIndexInRange(eqCap, bandLevel);
            }
            default:
                return false;
        }
        return false;
    }

    bool isPresetIndexInRange(const Equalizer::Capability& cap, int idx) const {
        const auto [min, max] =
                std::minmax_element(cap.presets.begin(), cap.presets.end(),
                                    [](const auto& a, const auto& b) { return a.index < b.index; });
        return idx >= min->index && idx <= max->index;
    }

    bool isBandIndexInRange(const Equalizer::Capability& cap,
                            const std::vector<Equalizer::BandLevel>& bandLevel) const {
        for (auto& it : bandLevel) {
            if (!isBandIndexInRange(cap, it.index)) return false;
        }
        return true;
    }

    bool isBandIndexInRange(const Equalizer::Capability& cap, int idx) const {
        const auto [min, max] =
                std::minmax_element(cap.bandFrequencies.begin(), cap.bandFrequencies.end(),
                                    [](const auto& a, const auto& b) { return a.index < b.index; });
        return idx >= min->index && idx <= max->index;
    }

  private:
    Equalizer::VendorExtension mVendorExtension;
    std::vector<std::pair<Equalizer::Tag, std::unique_ptr<Equalizer>>> mTags;

    bool validCapabilityTag(Capability& cap) { return cap.getTag() == Capability::equalizer; }

    void initParamSpecific() {
        Equalizer eq;
        eq.set<Equalizer::preset>(0);
        Parameter::Specific specific;
        specific.set<Parameter::Specific::equalizer>(eq);
        setSpecific(specific);
    }

    void CleanUp() { mTags.clear(); }
};

TEST_P(EqualizerParamTest, SetAndGetPreset) {
    EXPECT_NO_FATAL_FAILURE(addPresetParam(mParamPresetIndex));
    SetAndGetEqualizerParameters();
}

TEST_P(EqualizerParamTest, SetAndGetSingleBand) {
    Equalizer::BandLevel bandLevel = {mParamBandIndex, mParamBandLevel};
    std::vector<Equalizer::BandLevel> bandLevels;
    bandLevels.push_back(bandLevel);
    EXPECT_NO_FATAL_FAILURE(addBandLevelsParam(bandLevels));
    SetAndGetEqualizerParameters();
}

INSTANTIATE_TEST_SUITE_P(
        EqualizerTest, EqualizerParamTest,
        ::testing::Combine(
                testing::ValuesIn(android::getAidlHalInstanceNames(IFactory::descriptor)),
                testing::Range(kPresetIndexRange.first, kPresetIndexRange.second),
                testing::Range(kBandIndexRange.first, kBandIndexRange.second),
                testing::Range(kBandLevelRange.first, kBandLevelRange.second)),
        [](const testing::TestParamInfo<EqualizerParamTest::ParamType>& info) {
            std::string instance = std::get<PARAM_INSTANCE_NAME>(info.param);
            std::string presetIdx = std::to_string(std::get<PARAM_PRESET_INDEX>(info.param));
            std::string bandIdx = std::to_string(std::get<PARAM_BAND_INDEX>(info.param));
            std::string bandLevel = std::to_string(std::get<PARAM_BAND_LEVEL>(info.param));

            std::string name = instance + "_presetIndex" + presetIdx + "_bandIndex" + bandIdx +
                               "_bandLevel" + bandLevel;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EqualizerParamTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
