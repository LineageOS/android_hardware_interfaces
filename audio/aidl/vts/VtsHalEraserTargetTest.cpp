/*
 * Copyright (C) 2025 The Android Open Source Project
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
#include <cstddef>
#include <cstdio>
#include <future>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#define LOG_TAG "VtsHalEraserTest"
#include <aidl/Gtest.h>
#include <aidl/android/hardware/audio/effect/IEffect.h>
#include <aidl/android/hardware/audio/effect/IFactory.h>
#include <aidl/android/media/audio/eraser/BnEraserCallback.h>
#include <aidl/android/media/audio/eraser/ClassificationMetadata.h>
#include <aidl/android/media/audio/eraser/ClassificationMetadataList.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android-base/thread_annotations.h>
#include <android/binder_interface_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <gtest/gtest.h>
#include <stdio.h>

#include "audio_utils/mutex.h"
#include "audio_utils/sndfile.h"

#include "EffectHelper.h"
#include "TestUtils.h"

using namespace android;

using aidl::android::hardware::audio::common::getChannelCount;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::Eraser;
using aidl::android::hardware::audio::effect::getEffectTypeUuidEraser;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;
using aidl::android::media::audio::common::AudioFormatType;
using aidl::android::media::audio::common::PcmType;
using aidl::android::media::audio::eraser::BnEraserCallback;
using aidl::android::media::audio::eraser::ClassificationMetadata;
using aidl::android::media::audio::eraser::ClassificationMetadataList;
using aidl::android::media::audio::eraser::Mode;
using aidl::android::media::audio::eraser::SoundClassification;
using ::android::audio::utils::toString;
using android::hardware::audio::common::testing::detail::TestExecutionTracer;

class EraserCallback : public BnEraserCallback {
  public:
    ndk::ScopedAStatus onClassifierUpdate(int,
                                          const ClassificationMetadataList& metadataList) override {
        audio_utils::unique_lock lock(mMutex);
        mResults.push_back(metadataList);
        LOG(DEBUG) << " received metadata list " << metadataList.toString();
        mCv.notify_one();
        return ndk::ScopedAStatus::ok();
    }

    std::vector<ClassificationMetadataList> getResults() {
        audio_utils::unique_lock lock(mMutex);
        return mResults;
    }

  private:
    std::mutex mMutex;
    audio_utils::condition_variable mCv;
    std::vector<ClassificationMetadataList> mResults GUARDED_BY(mMutex);
};

class EraserTestHelper : public EffectHelper {
  public:
    EraserTestHelper(std::pair<std::shared_ptr<IFactory>, Descriptor> descPair)
        : mFactory(descPair.first) {
        mDescriptor = descPair.second;
    }

    void SetUpEraser() {
        ASSERT_NE(nullptr, mFactory);
        ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    }

    void TearDownEraser() {
        ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
        mOpenEffectReturn = IEffect::OpenEffectReturn{};
    }

    bool isModeSupported(Mode mode) const {
        if (!mEffect) return false;

        Parameter param;
        Eraser::Id eraserId = Eraser::Id::make<Eraser::Id::commonTag>(Eraser::capability);
        Parameter::Id capId = Parameter::Id::make<Parameter::Id::eraserTag>(eraserId);
        EXPECT_IS_OK(mEffect->getParameter(capId, &param));

        const auto specific = param.get<Parameter::specific>();
        const auto eraser = specific.get<Parameter::Specific::eraser>();
        const auto cap = eraser.get<Eraser::capability>();
        return std::find(cap.modes.begin(), cap.modes.end(), mode) != cap.modes.end();
    }

    bool setEraserMode(Mode mode) {
        if (!mEffect) return false;

        using EraserConfiguration = aidl::android::media::audio::eraser::Configuration;
        Eraser eraser = Eraser::make<Eraser::configuration>(EraserConfiguration({.mode = mode}));
        Parameter::Specific specific =
                Parameter::Specific::make<Parameter::Specific::eraser>(eraser);
        Parameter param = Parameter::make<Parameter::specific>(specific);
        EXPECT_IS_OK(mEffect->setParameter(param));
        return true;
    }

    static bool readWavFile(const std::string& aacFilePath, std::vector<float>* wavData) {
        if (aacFilePath.empty() || !wavData) {
            return false;
        }
        SF_INFO sfinfo;
        SNDFILE* sndfile = sf_open(aacFilePath.c_str(), SFM_READ, &sfinfo);
        if (!sndfile) {
            LOG(ERROR) << "Could not open wav file " << aacFilePath;
            return false;
        }
        if (sfinfo.channels > 2) {
            LOG(ERROR) << "Only support mono or stereo wav file";
            return false;
        }
        wavData->resize(sfinfo.frames * sfinfo.channels);
        sf_readf_float(sndfile, wavData->data(), sfinfo.frames);
        sf_close(sndfile);
        return true;
    }

    static constexpr long kInputFrameCount = 0x4000, kOutputFrameCount = 0x4000;
    const std::shared_ptr<IFactory> mFactory;
    std::shared_ptr<IEffect> mEffect;
    IEffect::OpenEffectReturn mOpenEffectReturn;
    static constexpr AudioChannelLayout kMonoChannel = AudioChannelLayout(
            std::in_place_index<static_cast<size_t>(AudioChannelLayout::layoutMask)>,
            AudioChannelLayout::LAYOUT_MONO);

  private:
    std::vector<std::pair<Eraser::Tag, Eraser>> mTags;
    void CleanUp() { mTags.clear(); }
};

enum ParamName { PARAM_INSTANCE_NAME, PARAM_AUDIO_FILE };

using EraserParamTestParam = std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>>;
class EraserParamTest : public ::testing::TestWithParam<EraserParamTestParam>,
                        public EraserTestHelper {
  public:
    EraserParamTest() : EraserTestHelper(std::get<PARAM_INSTANCE_NAME>(GetParam())) {}

    void SetUp() override { ASSERT_NO_FATAL_FAILURE(SetUpEraser()); }

    void TearDown() override { ASSERT_NO_FATAL_FAILURE(TearDownEraser()); }
};

TEST_P(EraserParamTest, OpenFailWithUnsupportedFormats) {
    Eraser::Id eraserId = Eraser::Id::make<Eraser::Id::commonTag>(Eraser::capability);
    Parameter::Id capId = Parameter::Id::make<Parameter::Id::eraserTag>(eraserId);
    Parameter capParam;

    IEffect::OpenEffectReturn ret;
    const Parameter::Common supported = createParamCommon();
    Parameter::Common unsupported = supported;
    unsupported.input.base.sampleRate = 48000;
    ASSERT_STATUS(EX_NULL_POINTER, mEffect->open(unsupported, std::nullopt, &ret));

    unsupported = supported;
    unsupported.input.base.channelMask = AudioChannelLayout::make<AudioChannelLayout::layoutMask>(
            AudioChannelLayout::LAYOUT_STEREO);
    unsupported.output.base.channelMask = AudioChannelLayout::make<AudioChannelLayout::layoutMask>(
            AudioChannelLayout::LAYOUT_STEREO);
    ASSERT_STATUS(EX_NULL_POINTER, mEffect->open(unsupported, std::nullopt, &ret));

    unsupported = supported;
    unsupported.input.base.format.type = AudioFormatType::NON_PCM;
    ASSERT_STATUS(EX_NULL_POINTER, mEffect->open(unsupported, std::nullopt, &ret));
}

TEST_P(EraserParamTest, OpenCloseSeq) {
    Parameter::Common common = createParamCommon();
    ASSERT_NO_FATAL_FAILURE(open(mEffect, common, std::nullopt, &mOpenEffectReturn, EX_NONE));
    ASSERT_NE(nullptr, mEffect);

    ASSERT_NO_FATAL_FAILURE(close(mEffect));
}

TEST_P(EraserParamTest, SetClassifierMode) {
    Parameter::Common common = createParamCommon();
    ASSERT_NO_FATAL_FAILURE(open(mEffect, common, std::nullopt, &mOpenEffectReturn, EX_NONE));
    ASSERT_NE(nullptr, mEffect);

    // eraser effect must support CLASSIFIER mode
    ASSERT_TRUE(isModeSupported(Mode::CLASSIFIER));
    ASSERT_TRUE(setEraserMode(Mode::CLASSIFIER));

    ASSERT_NO_FATAL_FAILURE(close(mEffect));
}

TEST_P(EraserParamTest, SetEraserModeIfSupported) {
    Parameter::Common common = createParamCommon();
    ASSERT_NO_FATAL_FAILURE(open(mEffect, common, std::nullopt, &mOpenEffectReturn, EX_NONE));
    ASSERT_NE(nullptr, mEffect);

    if (isModeSupported(Mode::ERASER)) {
        ASSERT_TRUE(setEraserMode(Mode::ERASER));
    } else {
        GTEST_SKIP() << "Eraser mode not supported, skipping test";
    }

    ASSERT_NO_FATAL_FAILURE(close(mEffect));
}

std::vector<std::pair<std::shared_ptr<IFactory>, Descriptor>> kDescPair;
INSTANTIATE_TEST_SUITE_P(EraserParamTest, EraserParamTest,
                         ::testing::Combine(testing::ValuesIn(
                                 kDescPair = EffectFactoryHelper::getAllEffectDescriptors(
                                         IFactory::descriptor, getEffectTypeUuidEraser()))),
                         [](const testing::TestParamInfo<EraserParamTest::ParamType>& info) {
                             auto descriptor = std::get<PARAM_INSTANCE_NAME>(info.param).second;
                             std::string name = getPrefix(descriptor);
                             std::replace_if(
                                     name.begin(), name.end(),
                                     [](const char c) { return !std::isalnum(c); }, '_');
                             return name;
                         });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EraserParamTest);

using EraserDataTestParam = std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>,
                                       std::pair<std::string, SoundClassification>>;
class EraserDataTest : public ::testing::TestWithParam<EraserDataTestParam>,
                       public EraserTestHelper {
  public:
    EraserDataTest()
        : EraserTestHelper(std::get<PARAM_INSTANCE_NAME>(GetParam())),
          mAudioFile(std::get<PARAM_AUDIO_FILE>(GetParam()).first),
          mExpectedClassification(std::get<PARAM_AUDIO_FILE>(GetParam()).second) {
        LOG(INFO) << " testing " << toString(mExpectedClassification) << " with " << mAudioFile;
    }

    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(SetUpEraser());

        Parameter::Common common =
                createParamCommon(AUDIO_SESSION_NONE, 15600 /*iFrameCount*/, 15600 /*oFrameCount*/);
        ASSERT_NO_FATAL_FAILURE(open(mEffect, common, std::nullopt, &mOpenEffectReturn, EX_NONE));
        ASSERT_NE(nullptr, mEffect);
    }

    void TearDown() override {
        ASSERT_NO_FATAL_FAILURE(close(mEffect));
        ASSERT_NO_FATAL_FAILURE(TearDownEraser());
    }
    const std::string mAudioFile;
    const SoundClassification mExpectedClassification;
};

TEST_P(EraserDataTest, ClassifySounds) {
    // eraser effect must support CLASSIFIER mode
    ASSERT_TRUE(isModeSupported(Mode::CLASSIFIER));

    auto callback = ndk::SharedRefBase::make<EraserCallback>();
    using EraserConfiguration = aidl::android::media::audio::eraser::Configuration;
    Eraser eraser = Eraser::make<Eraser::configuration>(
            EraserConfiguration({.mode = Mode::CLASSIFIER, .callback = callback}));
    Parameter::Specific specific = Parameter::Specific::make<Parameter::Specific::eraser>(eraser);
    Parameter param = Parameter::make<Parameter::specific>(specific);
    EXPECT_IS_OK(mEffect->setParameter(param));

    std::vector<float> wavData;
    ASSERT_TRUE(readWavFile(mAudioFile, &wavData));

    const auto channelCount = getChannelCount(kMonoChannel);
    ASSERT_NE(0ul, channelCount);
    std::vector<float> out(kOutputFrameCount * channelCount, 0);

    ASSERT_NO_FATAL_FAILURE(processInputAndWriteToOutput(wavData, out, mEffect, mOpenEffectReturn));

    // very loose check, make sure the classifier report at least one expected sound category
    auto results = callback->getResults();
    ASSERT_TRUE(results.size() >= 0);
    bool foundExpectedSound = false;
    // check the expected sound exist in the last result
    for (const auto& result : results) {
        for (const auto& metadata : result.metadatas) {
            // verify the sound category and confidence score with loose expectation
            if (metadata.classification.classification == mExpectedClassification) {
                foundExpectedSound = true;
                break;
            }
        }
        if (foundExpectedSound) {
            break;
        }
    }
    ASSERT_TRUE(foundExpectedSound);
}

[[clang::no_destroy]] static const std::vector<std::pair<std::string, SoundClassification>>
        kClassifierFileMap = {
                {"/data/local/tmp/speech.16khz.1ch.f32.wav", SoundClassification::HUMAN},
                {"/data/local/tmp/bird.16khz.1ch.f32.wav", SoundClassification::ANIMAL},
                {"/data/local/tmp/wind.16khz.1ch.f32.wav", SoundClassification::ENVIRONMENT},
                {"/data/local/tmp/motorcycle.16khz.1ch.f32.wav", SoundClassification::THINGS},
                {"/data/local/tmp/rain.16khz.1ch.f32.wav", SoundClassification::NATURE},
                {"/data/local/tmp/music.16khz.1ch.f32.wav", SoundClassification::MUSIC},
                {"/data/local/tmp/pinknoise.16khz.1ch.f32.wav", SoundClassification::AMBIGUOUS},
};

INSTANTIATE_TEST_SUITE_P(
        EraserDataTest, EraserDataTest,
        ::testing::Combine(
                testing::ValuesIn(kDescPair = EffectFactoryHelper::getAllEffectDescriptors(
                                          IFactory::descriptor, getEffectTypeUuidEraser())),
                testing::ValuesIn(kClassifierFileMap)),
        [](const testing::TestParamInfo<EraserDataTest::ParamType>& info) {
            auto descriptor = std::get<PARAM_INSTANCE_NAME>(info.param).second;
            std::string name =
                    getPrefix(descriptor) + "_" + std::get<PARAM_AUDIO_FILE>(info.param).first;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EraserDataTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::UnitTest::GetInstance()->listeners().Append(new TestExecutionTracer());
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
