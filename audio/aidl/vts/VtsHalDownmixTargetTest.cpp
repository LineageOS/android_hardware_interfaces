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

#include <audio_utils/ChannelMix.h>
#include "EffectHelper.h"

using namespace android;

using aidl::android::hardware::audio::common::getChannelCount;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::Downmix;
using aidl::android::hardware::audio::effect::getEffectTypeUuidDownmix;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;
using android::audio_utils::channels::ChannelMix;
using android::hardware::audio::common::testing::detail::TestExecutionTracer;

// Testing for enum values
static const std::vector<Downmix::Type> kTypeValues = {ndk::enum_range<Downmix::Type>().begin(),
                                                       ndk::enum_range<Downmix::Type>().end()};

// Testing for supported layouts from AudioChannelLayout.h
static const std::vector<int32_t> kLayoutValues = {
        AudioChannelLayout::LAYOUT_STEREO,        AudioChannelLayout::LAYOUT_2POINT1,
        AudioChannelLayout::LAYOUT_TRI,           AudioChannelLayout::LAYOUT_TRI_BACK,
        AudioChannelLayout::LAYOUT_3POINT1,       AudioChannelLayout::LAYOUT_2POINT0POINT2,
        AudioChannelLayout::LAYOUT_2POINT1POINT2, AudioChannelLayout::LAYOUT_3POINT0POINT2,
        AudioChannelLayout::LAYOUT_3POINT1POINT2, AudioChannelLayout::LAYOUT_QUAD,
        AudioChannelLayout::LAYOUT_QUAD_SIDE,     AudioChannelLayout::LAYOUT_SURROUND,
        AudioChannelLayout::LAYOUT_PENTA,         AudioChannelLayout::LAYOUT_5POINT1,
        AudioChannelLayout::LAYOUT_5POINT1_SIDE,  AudioChannelLayout::LAYOUT_5POINT1POINT2,
        AudioChannelLayout::LAYOUT_5POINT1POINT4, AudioChannelLayout::LAYOUT_6POINT1,
        AudioChannelLayout::LAYOUT_7POINT1,       AudioChannelLayout::LAYOUT_7POINT1POINT2,
        AudioChannelLayout::LAYOUT_7POINT1POINT4, AudioChannelLayout::LAYOUT_9POINT1POINT4,
        AudioChannelLayout::LAYOUT_9POINT1POINT6, AudioChannelLayout::LAYOUT_13POINT_360RA,
        AudioChannelLayout::LAYOUT_22POINT2};

static const std::vector<int32_t> kChannels = {
        AudioChannelLayout::CHANNEL_FRONT_LEFT,
        AudioChannelLayout::CHANNEL_FRONT_RIGHT,
        AudioChannelLayout::CHANNEL_FRONT_CENTER,
        AudioChannelLayout::CHANNEL_LOW_FREQUENCY,
        AudioChannelLayout::CHANNEL_BACK_LEFT,
        AudioChannelLayout::CHANNEL_BACK_RIGHT,
        AudioChannelLayout::CHANNEL_BACK_CENTER,
        AudioChannelLayout::CHANNEL_SIDE_LEFT,
        AudioChannelLayout::CHANNEL_SIDE_RIGHT,
        AudioChannelLayout::CHANNEL_FRONT_LEFT_OF_CENTER,
        AudioChannelLayout::CHANNEL_FRONT_RIGHT_OF_CENTER,
        AudioChannelLayout::CHANNEL_TOP_CENTER,
        AudioChannelLayout::CHANNEL_TOP_FRONT_LEFT,
        AudioChannelLayout::CHANNEL_TOP_FRONT_CENTER,
        AudioChannelLayout::CHANNEL_TOP_FRONT_RIGHT,
        AudioChannelLayout::CHANNEL_TOP_BACK_LEFT,
        AudioChannelLayout::CHANNEL_TOP_BACK_CENTER,
        AudioChannelLayout::CHANNEL_TOP_BACK_RIGHT,
        AudioChannelLayout::CHANNEL_TOP_SIDE_LEFT,
        AudioChannelLayout::CHANNEL_TOP_SIDE_RIGHT,
        AudioChannelLayout::CHANNEL_BOTTOM_FRONT_LEFT,
        AudioChannelLayout::CHANNEL_BOTTOM_FRONT_CENTER,
        AudioChannelLayout::CHANNEL_BOTTOM_FRONT_RIGHT,
        AudioChannelLayout::CHANNEL_LOW_FREQUENCY_2,
        AudioChannelLayout::CHANNEL_FRONT_WIDE_LEFT,
        AudioChannelLayout::CHANNEL_FRONT_WIDE_RIGHT,
};

class DownmixEffectHelper : public EffectHelper {
  public:
    void SetUpDownmix(int32_t inputBufferLayout = AudioChannelLayout::LAYOUT_STEREO) {
        ASSERT_NE(nullptr, mFactory);
        ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));

        AudioChannelLayout inputChannelLayout =
                AudioChannelLayout::make<AudioChannelLayout::layoutMask>(inputBufferLayout);

        Parameter::Specific specific = getDefaultParamSpecific();
        Parameter::Common common = EffectHelper::createParamCommon(
                0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */,
                kInputFrameCount /* iFrameCount */, kOutputFrameCount /* oFrameCount */,
                inputChannelLayout);
        ASSERT_NO_FATAL_FAILURE(open(mEffect, common, specific, &mOpenEffectReturn, EX_NONE));
        ASSERT_NE(nullptr, mEffect);
    }

    void TearDownDownmix() {
        ASSERT_NO_FATAL_FAILURE(close(mEffect));
        ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
        mOpenEffectReturn = IEffect::OpenEffectReturn{};
    }

    Parameter createDownmixParam(Downmix::Type type) {
        return Parameter::make<Parameter::specific>(
                Parameter::Specific::make<Parameter::Specific::downmix>(
                        Downmix::make<Downmix::type>(type)));
    }
    void setParameters(Downmix::Type type) {
        // set parameter
        auto param = createDownmixParam(type);
        EXPECT_STATUS(EX_NONE, mEffect->setParameter(param)) << param.toString();
    }

    void validateParameters(Downmix::Type type) {
        auto leId = Downmix::Id::make<Downmix::Id::commonTag>(Downmix::Tag(Downmix::type));
        auto id = Parameter::Id::make<Parameter::Id::downmixTag>(leId);
        // get parameter
        Parameter getParam;
        EXPECT_STATUS(EX_NONE, mEffect->getParameter(id, &getParam));
        auto expectedParam = createDownmixParam(type);
        EXPECT_EQ(expectedParam, getParam) << "\nexpectedParam:" << expectedParam.toString()
                                           << "\ngetParam:" << getParam.toString();
    }

    Parameter::Specific getDefaultParamSpecific() {
        Downmix dm = Downmix::make<Downmix::type>(Downmix::Type::STRIP);
        Parameter::Specific specific = Parameter::Specific::make<Parameter::Specific::downmix>(dm);
        return specific;
    }

    void setDataTestParams(int32_t layoutType) {
        mInputBuffer.resize(kBufferSize);
        mOutputBuffer.resize(kBufferSize);

        // Get the number of channels used
        mInputChannelCount = getChannelCount(
                AudioChannelLayout::make<AudioChannelLayout::layoutMask>(layoutType));

        // In case of downmix, output is always configured to stereo layout.
        mOutputBufferSize = (mInputBuffer.size() / mInputChannelCount) * kOutputChannelCount;
    }

    // Generate mInputBuffer values between -kMaxDownmixSample to kMaxDownmixSample
    void generateInputBuffer(size_t position, bool isStrip) {
        size_t increment;
        if (isStrip)
            // Fill input at all the channels
            increment = 1;
        else
            // Fill input at only one channel
            increment = mInputChannelCount;

        for (size_t i = position; i < mInputBuffer.size(); i += increment) {
            mInputBuffer[i] =
                    ((static_cast<float>(std::rand()) / RAND_MAX) * 2 - 1) * kMaxDownmixSample;
        }
    }

    bool isLayoutValid(int32_t inputLayout) {
        if (inputLayout & kMaxChannelMask) {
            return false;
        }
        return true;
    }

    static constexpr long kInputFrameCount = 100, kOutputFrameCount = 100;
    std::shared_ptr<IFactory> mFactory;
    Descriptor mDescriptor;
    std::shared_ptr<IEffect> mEffect;
    IEffect::OpenEffectReturn mOpenEffectReturn;

    std::vector<float> mInputBuffer;
    std::vector<float> mOutputBuffer;
    size_t mInputChannelCount;
    size_t mOutputBufferSize;
    static constexpr size_t kBufferSize = 128;
    static constexpr float kMaxDownmixSample = 1;
    static constexpr int kOutputChannelCount = 2;
    // Mask for layouts greater than MAX_INPUT_CHANNELS_SUPPORTED
    static constexpr int32_t kMaxChannelMask =
            ~((1 << ChannelMix<AUDIO_CHANNEL_OUT_STEREO>::MAX_INPUT_CHANNELS_SUPPORTED) - 1);
};

/**
 * Here we focus on specific parameter checking, general IEffect interfaces testing performed in
 * VtsAudioEffectTargetTest.
 */
enum ParamName { PARAM_INSTANCE_NAME, PARAM_TYPE };

using DownmixParamTestParam =
        std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, Downmix::Type>;

class DownmixParamTest : public ::testing::TestWithParam<DownmixParamTestParam>,
                         public DownmixEffectHelper {
  public:
    DownmixParamTest() : mParamType(std::get<PARAM_TYPE>(GetParam())) {
        std::tie(mFactory, mDescriptor) = std::get<PARAM_INSTANCE_NAME>(GetParam());
    }

    void SetUp() override { SetUpDownmix(); }

    void TearDown() override { TearDownDownmix(); }

    const Downmix::Type mParamType;
};

TEST_P(DownmixParamTest, SetAndGetType) {
    ASSERT_NO_FATAL_FAILURE(setParameters(mParamType));
    ASSERT_NO_FATAL_FAILURE(validateParameters(mParamType));
}

enum FoldParamName { FOLD_INSTANCE_NAME, FOLD_INPUT_LAYOUT, FOLD_TEST_CHANNEL };

using DownmixDataTestParamFold =
        std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int32_t>;

class DownmixFoldDataTest : public ::testing::TestWithParam<DownmixDataTestParamFold>,
                            public DownmixEffectHelper {
  public:
    DownmixFoldDataTest() : mInputChannelLayout(std::get<FOLD_INPUT_LAYOUT>(GetParam())) {
        std::tie(mFactory, mDescriptor) = std::get<FOLD_INSTANCE_NAME>(GetParam());
    }

    void SetUp() override {
        SetUpDownmix(mInputChannelLayout);
        if (!isLayoutValid(mInputChannelLayout)) {
            GTEST_SKIP() << "Layout not supported \n";
        }
        setDataTestParams(mInputChannelLayout);
    }

    void TearDown() override { TearDownDownmix(); }

    void checkAtLeft(int32_t position) {
        for (size_t i = 0, j = position; i < mOutputBufferSize;
             i += kOutputChannelCount, j += mInputChannelCount) {
            // Validate Left channel has audio
            if (mInputBuffer[j] != 0) {
                ASSERT_NE(mOutputBuffer[i], 0);
            } else {
                // No change in output when input is 0
                ASSERT_EQ(mOutputBuffer[i], mInputBuffer[j]);
            }
            // Validate Right channel has no audio
            ASSERT_EQ(mOutputBuffer[i + 1], 0);
        }
    }

    void checkAtRight(int32_t position) {
        for (size_t i = 0, j = position; i < mOutputBufferSize;
             i += kOutputChannelCount, j += mInputChannelCount) {
            // Validate Left channel has no audio
            ASSERT_EQ(mOutputBuffer[i], 0);
            // Validate Right channel has audio
            if (mInputBuffer[j] != 0) {
                ASSERT_NE(mOutputBuffer[i + 1], 0);
            } else {
                // No change in output when input is 0
                ASSERT_EQ(mOutputBuffer[i + 1], mInputBuffer[j]);
            }
        }
    }

    void checkAtCenter(size_t position) {
        for (size_t i = 0, j = position; i < mOutputBufferSize;
             i += kOutputChannelCount, j += mInputChannelCount) {
            // Validate both channels have audio
            if (mInputBuffer[j] != 0) {
                ASSERT_NE(mOutputBuffer[i], 0);
                ASSERT_NE(mOutputBuffer[i + 1], 0);

            } else {
                // No change in output when input is 0
                ASSERT_EQ(mOutputBuffer[i], mInputBuffer[j]);
                ASSERT_EQ(mOutputBuffer[i + 1], mInputBuffer[j]);
            }
        }
    }

    void validateOutput(int32_t channel, size_t position) {
        switch (channel) {
            case AudioChannelLayout::CHANNEL_FRONT_LEFT:
            case AudioChannelLayout::CHANNEL_BACK_LEFT:
            case AudioChannelLayout::CHANNEL_SIDE_LEFT:
            case AudioChannelLayout::CHANNEL_TOP_FRONT_LEFT:
            case AudioChannelLayout::CHANNEL_BOTTOM_FRONT_LEFT:
            case AudioChannelLayout::CHANNEL_TOP_BACK_LEFT:
            case AudioChannelLayout::CHANNEL_FRONT_WIDE_LEFT:
            case AudioChannelLayout::CHANNEL_TOP_SIDE_LEFT:
                checkAtLeft(position);
                break;

            case AudioChannelLayout::CHANNEL_FRONT_RIGHT:
            case AudioChannelLayout::CHANNEL_BACK_RIGHT:
            case AudioChannelLayout::CHANNEL_SIDE_RIGHT:
            case AudioChannelLayout::CHANNEL_TOP_FRONT_RIGHT:
            case AudioChannelLayout::CHANNEL_BOTTOM_FRONT_RIGHT:
            case AudioChannelLayout::CHANNEL_TOP_BACK_RIGHT:
            case AudioChannelLayout::CHANNEL_FRONT_WIDE_RIGHT:
            case AudioChannelLayout::CHANNEL_TOP_SIDE_RIGHT:
            case AudioChannelLayout::CHANNEL_LOW_FREQUENCY_2:
                checkAtRight(position);
                break;

            case AudioChannelLayout::CHANNEL_FRONT_CENTER:
            case AudioChannelLayout::CHANNEL_BACK_CENTER:
            case AudioChannelLayout::CHANNEL_TOP_FRONT_CENTER:
            case AudioChannelLayout::CHANNEL_BOTTOM_FRONT_CENTER:
            case AudioChannelLayout::CHANNEL_FRONT_LEFT_OF_CENTER:
            case AudioChannelLayout::CHANNEL_FRONT_RIGHT_OF_CENTER:
            case AudioChannelLayout::CHANNEL_TOP_CENTER:
            case AudioChannelLayout::CHANNEL_TOP_BACK_CENTER:
                checkAtCenter(position);
                break;

            case AudioChannelLayout::CHANNEL_LOW_FREQUENCY:
                // If CHANNEL_LOW_FREQUENCY_2 is supported
                if (mInputChannelLayout & AudioChannelLayout::CHANNEL_LOW_FREQUENCY_2) {
                    // Validate that only Left channel has audio
                    checkAtLeft(position);
                } else {
                    // Validate that both channels have audio
                    checkAtCenter(position);
                }
                break;
        }
    }

    std::set<int32_t> getInputChannelLayouts() {
        std::set<int32_t> supportedChannels;
        for (int32_t channel : kChannels) {
            if ((mInputChannelLayout & channel) == channel) {
                supportedChannels.insert(channel);
            }
        }
        return supportedChannels;
    }

    int32_t mInputChannelLayout;
};

TEST_P(DownmixFoldDataTest, DownmixProcessData) {
    // Set FOLD type parameter
    ASSERT_NO_FATAL_FAILURE(setParameters(Downmix::Type::FOLD));

    // Get all the channels from input layout
    std::set<int32_t> supportedChannels = getInputChannelLayouts();

    for (int32_t channel : supportedChannels) {
        size_t position = std::distance(supportedChannels.begin(), supportedChannels.find(channel));
        generateInputBuffer(position, false /*isStripe*/);
        ASSERT_NO_FATAL_FAILURE(
                processAndWriteToOutput(mInputBuffer, mOutputBuffer, mEffect, &mOpenEffectReturn));
        validateOutput(channel, position);
        std::fill(mInputBuffer.begin(), mInputBuffer.end(), 0);
    }
}

enum StripParamName { STRIP_INSTANCE_NAME, STRIP_INPUT_LAYOUT };

using DownmixStripDataTestParam =
        std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>, int32_t>;

class DownmixStripDataTest : public ::testing::TestWithParam<DownmixStripDataTestParam>,
                             public DownmixEffectHelper {
  public:
    DownmixStripDataTest() : mInputChannelLayout(std::get<STRIP_INPUT_LAYOUT>(GetParam())) {
        std::tie(mFactory, mDescriptor) = std::get<STRIP_INSTANCE_NAME>(GetParam());
    }

    void SetUp() override {
        SetUpDownmix(mInputChannelLayout);
        if (!isLayoutValid(mInputChannelLayout)) {
            GTEST_SKIP() << "Layout not supported \n";
        }
        setDataTestParams(mInputChannelLayout);
    }

    void TearDown() override { TearDownDownmix(); }

    void validateOutput() {
        ASSERT_EQ(kBufferSize, mInputBuffer.size());
        ASSERT_GE(kBufferSize, mOutputBufferSize);
        for (size_t i = 0, j = 0; i < kBufferSize && j < mOutputBufferSize;
             i += mInputChannelCount, j += kOutputChannelCount) {
            ASSERT_EQ(mOutputBuffer[j], mInputBuffer[i]);
            ASSERT_EQ(mOutputBuffer[j + 1], mInputBuffer[i + 1]);
        }
        for (size_t i = mOutputBufferSize; i < kBufferSize; i++) {
            ASSERT_EQ(mOutputBuffer[i], mInputBuffer[i]);
        }
    }

    int32_t mInputChannelLayout;
};

TEST_P(DownmixStripDataTest, DownmixProcessData) {
    // Set STRIP type parameter
    ASSERT_NO_FATAL_FAILURE(setParameters(Downmix::Type::STRIP));

    // Generate input buffer, call process and compare outputs
    generateInputBuffer(0 /*position*/, true /*isStripe*/);
    ASSERT_NO_FATAL_FAILURE(
            processAndWriteToOutput(mInputBuffer, mOutputBuffer, mEffect, &mOpenEffectReturn));
    validateOutput();
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

INSTANTIATE_TEST_SUITE_P(
        DownmixTest, DownmixFoldDataTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidDownmix())),
                           testing::ValuesIn(kLayoutValues)),
        [](const testing::TestParamInfo<DownmixFoldDataTest::ParamType>& info) {
            auto descriptor = std::get<FOLD_INSTANCE_NAME>(info.param).second;
            std::string layout = std::to_string(std::get<FOLD_INPUT_LAYOUT>(info.param));
            std::string name = getPrefix(descriptor) + "_fold" + "_layout" + layout;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DownmixFoldDataTest);

INSTANTIATE_TEST_SUITE_P(
        DownmixTest, DownmixStripDataTest,
        ::testing::Combine(testing::ValuesIn(EffectFactoryHelper::getAllEffectDescriptors(
                                   IFactory::descriptor, getEffectTypeUuidDownmix())),
                           testing::ValuesIn(kLayoutValues)),
        [](const testing::TestParamInfo<DownmixStripDataTest::ParamType>& info) {
            auto descriptor = std::get<STRIP_INSTANCE_NAME>(info.param).second;
            std::string layout =
                    std::to_string(static_cast<int>(std::get<STRIP_INPUT_LAYOUT>(info.param)));
            std::string name = getPrefix(descriptor) + "_strip" + "_layout" + layout;
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(DownmixStripDataTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::UnitTest::GetInstance()->listeners().Append(new TestExecutionTracer());
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
