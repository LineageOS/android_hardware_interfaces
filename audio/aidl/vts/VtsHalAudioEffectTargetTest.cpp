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

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define LOG_TAG "VtsHalAudioEffect"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android/binder_interface_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <aidl/android/hardware/audio/effect/IEffect.h>
#include <aidl/android/hardware/audio/effect/IFactory.h>
#include <aidl/android/media/audio/common/AudioChannelLayout.h>
#include <aidl/android/media/audio/common/AudioDeviceType.h>

#include "AudioHalBinderServiceUtil.h"
#include "EffectFactoryHelper.h"
#include "TestUtils.h"

using namespace android;

using ndk::ScopedAStatus;

using aidl::android::hardware::audio::effect::CommandId;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;
using aidl::android::hardware::audio::effect::State;
using aidl::android::media::audio::common::AudioChannelLayout;
using aidl::android::media::audio::common::AudioDeviceType;

class AudioEffect : public testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        ASSERT_NO_FATAL_FAILURE(mFactoryHelper.ConnectToFactoryService());
        CreateEffects();
        initParamCommon();
        initParamSpecific();
    }

    void TearDown() override {
        CloseEffects();
        DestroyEffects();
    }

    void OpenEffects() {
        auto open = [&](const std::shared_ptr<IEffect>& effect) {
            IEffect::OpenEffectReturn ret;
            EXPECT_IS_OK(effect->open(mCommon, mSpecific, &ret));
        };
        EXPECT_NO_FATAL_FAILURE(ForEachEffect(open));
    }

    void CloseEffects(const binder_status_t status = EX_NONE) {
        auto close = [&](const std::shared_ptr<IEffect>& effect) {
            EXPECT_STATUS(status, effect->close());
        };

        EXPECT_NO_FATAL_FAILURE(ForEachEffect(close));
    }

    void CreateEffects(const int n = 1) {
        for (int i = 0; i < n; i++) {
            ASSERT_NO_FATAL_FAILURE(mFactoryHelper.QueryAndCreateAllEffects());
        }
    }

    void DestroyEffects(const binder_status_t status = EX_NONE, const int remaining = 0) {
        ASSERT_NO_FATAL_FAILURE(mFactoryHelper.DestroyEffects(status, remaining));
    }

    void GetEffectDescriptors() {
        auto get = [](const std::shared_ptr<IEffect>& effect) {
            Descriptor desc;
            EXPECT_IS_OK(effect->getDescriptor(&desc));
        };
        EXPECT_NO_FATAL_FAILURE(ForEachEffect(get));
    }

    void CommandEffects(CommandId command) {
        auto close = [&](const std::shared_ptr<IEffect>& effect) {
            EXPECT_IS_OK(effect->command(command));
        };
        EXPECT_NO_FATAL_FAILURE(ForEachEffect(close));
    }

    void CommandEffectsExpectStatus(CommandId command, const binder_status_t status) {
        auto func = [&](const std::shared_ptr<IEffect>& effect) {
            EXPECT_STATUS(status, effect->command(command));
        };
        EXPECT_NO_FATAL_FAILURE(ForEachEffect(func));
    }

    void ExpectState(State expected) {
        auto get = [&](const std::shared_ptr<IEffect>& effect) {
            State state = State::INIT;
            EXPECT_IS_OK(effect->getState(&state));
            EXPECT_EQ(expected, state);
        };
        EXPECT_NO_FATAL_FAILURE(ForEachEffect(get));
    }

    void SetParameter() {
        auto func = [&](const std::shared_ptr<IEffect>& effect) {
            Parameter param;
            param.set<Parameter::common>(mCommon);
            EXPECT_IS_OK(effect->setParameter(param));
        };
        EXPECT_NO_FATAL_FAILURE(ForEachEffect(func));
    }

    void VerifyParameters() {
        auto func = [&](const std::shared_ptr<IEffect>& effect) {
            Parameter paramCommonGet = Parameter(), paramCommonExpect = Parameter();
            Parameter::Id id;
            id.set<Parameter::Id::commonTag>(0);
            paramCommonExpect.set<Parameter::common>(mCommon);
            EXPECT_IS_OK(effect->getParameter(id, &paramCommonGet));
            EXPECT_EQ(paramCommonExpect, paramCommonGet)
                    << paramCommonExpect.toString() << " vs " << paramCommonGet.toString();
        };
        EXPECT_NO_FATAL_FAILURE(ForEachEffect(func));
    }

    template <typename Functor>
    void ForEachEffect(Functor functor) {
        auto effectMap = mFactoryHelper.GetEffectMap();
        for (const auto& it : effectMap) {
            SCOPED_TRACE(it.second.toString());
            functor(it.first);
        }
    }

    void initParamCommon(int session = -1, int ioHandle = -1,
                         AudioDeviceType deviceType = AudioDeviceType::NONE,
                         int iSampleRate = 48000, int oSampleRate = 48000, long iFrameCount = 0x100,
                         long oFrameCount = 0x100) {
        mCommon.session = session;
        mCommon.ioHandle = ioHandle;
        mCommon.device.type = deviceType;
        mCommon.input.base.sampleRate = iSampleRate;
        mCommon.input.base.channelMask = mInputChannelLayout;
        mCommon.input.frameCount = iFrameCount;
        mCommon.output.base.sampleRate = oSampleRate;
        mCommon.output.base.channelMask = mOutputChannelLayout;
        mCommon.output.frameCount = oFrameCount;
    }

    void initParamSpecific(Parameter::Specific::Tag tag = Parameter::Specific::equalizer) {
        switch (tag) {
            case Parameter::Specific::equalizer:
                mSpecific.set<Parameter::Specific::equalizer>();
                break;
            default:
                return;
        }
    }

    void setInputChannelLayout(AudioChannelLayout input) { mInputChannelLayout = input; }
    void setOutputChannelLayout(AudioChannelLayout output) { mOutputChannelLayout = output; }

    EffectFactoryHelper mFactoryHelper = EffectFactoryHelper(GetParam());

  private:
    AudioChannelLayout mInputChannelLayout =
            AudioChannelLayout::make<AudioChannelLayout::layoutMask>(
                    AudioChannelLayout::LAYOUT_STEREO);
    AudioChannelLayout mOutputChannelLayout =
            AudioChannelLayout::make<AudioChannelLayout::layoutMask>(
                    AudioChannelLayout::LAYOUT_STEREO);

    Parameter::Common mCommon;
    Parameter::Specific mSpecific;
    static IEffect::OpenEffectReturn mOpenReturn;
};

TEST_P(AudioEffect, OpenEffectTest) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
}

TEST_P(AudioEffect, OpenAndCloseEffect) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

TEST_P(AudioEffect, CloseUnopenedEffectTest) {
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

TEST_P(AudioEffect, DoubleOpenCloseEffects) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(CloseEffects());

    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(CloseEffects());

    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

TEST_P(AudioEffect, GetDescriptors) {
    EXPECT_NO_FATAL_FAILURE(GetEffectDescriptors());
}

TEST_P(AudioEffect, DescriptorIdExistAndUnique) {
    auto checker = [&](const std::shared_ptr<IEffect>& effect) {
        Descriptor desc;
        std::vector<Descriptor::Identity> idList;
        EXPECT_IS_OK(effect->getDescriptor(&desc));
        mFactoryHelper.QueryEffects(desc.common.id.type, desc.common.id.uuid, &idList);
        EXPECT_EQ(idList.size(), 1UL);
    };
    EXPECT_NO_FATAL_FAILURE(ForEachEffect(checker));

    // Check unique with a set
    auto stringHash = [](const Descriptor::Identity& id) {
        return std::hash<std::string>()(id.toString());
    };
    auto vec = mFactoryHelper.GetCompleteEffectIdList();
    std::unordered_set<Descriptor::Identity, decltype(stringHash)> idSet(0, stringHash);
    for (auto it : vec) {
        EXPECT_EQ(idSet.count(it), 0UL);
        idSet.insert(it);
    }
}

/// State testing.
// An effect instance is in INIT state by default after it was created.
TEST_P(AudioEffect, InitStateAfterCreation) {
    ExpectState(State::INIT);
}

// An effect instance transfer to INIT state after it was open successfully with IEffect.open().
TEST_P(AudioEffect, IdleStateAfterOpen) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

// An effect instance is in PROCESSING state after it receive an START command.
TEST_P(AudioEffect, ProcessingStateAfterStart) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::START));
    ExpectState(State::PROCESSING);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::STOP));
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

// An effect instance transfer to IDLE state after Command.Id.STOP in PROCESSING state.
TEST_P(AudioEffect, IdleStateAfterStop) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::START));
    ExpectState(State::PROCESSING);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::STOP));
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

// An effect instance transfer to IDLE state after Command.Id.RESET in PROCESSING state.
TEST_P(AudioEffect, IdleStateAfterReset) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::START));
    ExpectState(State::PROCESSING);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::RESET));
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

// An effect instance transfer to INIT if instance receive a close() call.
TEST_P(AudioEffect, InitStateAfterClose) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::START));
    ExpectState(State::PROCESSING);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::STOP));
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
    ExpectState(State::INIT);
}

// An effect instance shouldn't accept any command before open.
TEST_P(AudioEffect, NoCommandAcceptedBeforeOpen) {
    ExpectState(State::INIT);
    EXPECT_NO_FATAL_FAILURE(CommandEffectsExpectStatus(CommandId::START, EX_ILLEGAL_STATE));
    EXPECT_NO_FATAL_FAILURE(CommandEffectsExpectStatus(CommandId::STOP, EX_ILLEGAL_STATE));
    EXPECT_NO_FATAL_FAILURE(CommandEffectsExpectStatus(CommandId::RESET, EX_ILLEGAL_STATE));
    ExpectState(State::INIT);
}

// No-op when receive STOP command in IDLE state.
TEST_P(AudioEffect, StopCommandInIdleStateNoOp) {
    ExpectState(State::INIT);
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::STOP));
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

// No-op when receive STOP command in IDLE state.
TEST_P(AudioEffect, ResetCommandInIdleStateNoOp) {
    ExpectState(State::INIT);
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::RESET));
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

// Repeat START and STOP command.
TEST_P(AudioEffect, RepeatStartAndStop) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::START));
    ExpectState(State::PROCESSING);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::STOP));
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::START));
    ExpectState(State::PROCESSING);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::STOP));
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

// Repeat START and RESET command.
TEST_P(AudioEffect, RepeatStartAndReset) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::START));
    ExpectState(State::PROCESSING);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::RESET));
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::START));
    ExpectState(State::PROCESSING);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::RESET));
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

// Repeat START and STOP command, try to close at PROCESSING state.
TEST_P(AudioEffect, CloseProcessingStateEffects) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::START));
    ExpectState(State::PROCESSING);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::STOP));
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::START));
    ExpectState(State::PROCESSING);
    EXPECT_NO_FATAL_FAILURE(CloseEffects(EX_ILLEGAL_STATE));
    // cleanup
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::STOP));
    ExpectState(State::IDLE);
}

// Expect EX_ILLEGAL_STATE if the effect instance is not in a proper state to be destroyed.
TEST_P(AudioEffect, DestroyOpenEffects) {
    // cleanup all effects.
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
    ASSERT_NO_FATAL_FAILURE(DestroyEffects());

    // open effects, destroy without close, expect to get EX_ILLEGAL_STATE status.
    EXPECT_NO_FATAL_FAILURE(CreateEffects());
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(DestroyEffects(EX_ILLEGAL_STATE, 1));
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

/// Parameter testing.
// Verify parameters pass in open can be successfully get.
TEST_P(AudioEffect, VerifyParametersAfterOpen) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(VerifyParameters());
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

// Verify parameters pass in set can be successfully get.
TEST_P(AudioEffect, SetAndGetParameter) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(VerifyParameters());
    initParamCommon(1 /* session */, 1 /* ioHandle */, AudioDeviceType::IN_DEFAULT /* deviceType */,
                    44100 /* iSampleRate */, 44100 /* oSampleRate */);
    EXPECT_NO_FATAL_FAILURE(SetParameter());
    EXPECT_NO_FATAL_FAILURE(VerifyParameters());
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

// Verify parameters pass in set can be successfully get.
TEST_P(AudioEffect, SetAndGetParameterInProcessing) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(VerifyParameters());
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::START));
    ExpectState(State::PROCESSING);
    initParamCommon(1 /* session */, 1 /* ioHandle */, AudioDeviceType::IN_DEFAULT /* deviceType */,
                    44100 /* iSampleRate */, 44100 /* oSampleRate */);
    EXPECT_NO_FATAL_FAILURE(SetParameter());
    EXPECT_NO_FATAL_FAILURE(VerifyParameters());
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::STOP));
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

// Parameters kept after reset.
TEST_P(AudioEffect, ResetAndVerifyParameter) {
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    EXPECT_NO_FATAL_FAILURE(VerifyParameters());
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::START));
    ExpectState(State::PROCESSING);
    initParamCommon(1 /* session */, 1 /* ioHandle */, AudioDeviceType::IN_DEFAULT /* deviceType */,
                    44100 /* iSampleRate */, 44100 /* oSampleRate */);
    EXPECT_NO_FATAL_FAILURE(SetParameter());
    EXPECT_NO_FATAL_FAILURE(VerifyParameters());
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::RESET));
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(VerifyParameters());
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

// Multiple instances of same implementation running.
TEST_P(AudioEffect, MultipleInstancesRunning) {
    EXPECT_NO_FATAL_FAILURE(CreateEffects(3));
    ExpectState(State::INIT);
    EXPECT_NO_FATAL_FAILURE(OpenEffects());
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::START));
    ExpectState(State::PROCESSING);
    initParamCommon(1 /* session */, 1 /* ioHandle */, AudioDeviceType::IN_DEFAULT /* deviceType */,
                    44100 /* iSampleRate */, 44100 /* oSampleRate */);
    EXPECT_NO_FATAL_FAILURE(SetParameter());
    EXPECT_NO_FATAL_FAILURE(VerifyParameters());
    EXPECT_NO_FATAL_FAILURE(CommandEffects(CommandId::STOP));
    ExpectState(State::IDLE);
    EXPECT_NO_FATAL_FAILURE(VerifyParameters());
    EXPECT_NO_FATAL_FAILURE(CloseEffects());
}

INSTANTIATE_TEST_SUITE_P(AudioEffectTest, AudioEffect,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IFactory::descriptor)),
                         android::PrintInstanceNameToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AudioEffect);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}