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

#define LOG_TAG "VtsHalAudioEffectTargetTest"

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android/binder_interface_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <Utils.h>
#include <aidl/android/hardware/audio/effect/IEffect.h>
#include <aidl/android/hardware/audio/effect/IFactory.h>
#include <aidl/android/media/audio/common/AudioDeviceType.h>

#include "AudioHalBinderServiceUtil.h"
#include "EffectHelper.h"
#include "TestUtils.h"

using namespace android;

using ndk::ScopedAStatus;

using aidl::android::hardware::audio::effect::CommandId;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;
using aidl::android::hardware::audio::effect::State;
using aidl::android::media::audio::common::AudioDeviceType;

class AudioEffectTest : public testing::TestWithParam<std::string>, public EffectHelper {
  public:
    AudioEffectTest() : EffectHelper(GetParam()) {}

    void SetUp() override {
        CreateEffects();
        initParamCommonFormat();
        initParamCommon();
        // initParamSpecific();
    }

    void TearDown() override {
        CloseEffects();
        DestroyEffects();
    }
};

TEST_P(AudioEffectTest, OpenEffectTest) {
    OpenEffects();
}

TEST_P(AudioEffectTest, OpenAndCloseEffect) {
    OpenEffects();
    CloseEffects();
}

TEST_P(AudioEffectTest, CloseUnopenedEffectTest) {
    CloseEffects();
}

TEST_P(AudioEffectTest, DoubleOpenCloseEffects) {
    OpenEffects();
    CloseEffects();
    OpenEffects();
    CloseEffects();

    OpenEffects();
    OpenEffects();
    CloseEffects();

    OpenEffects();
    CloseEffects();
    CloseEffects();
}

TEST_P(AudioEffectTest, GetDescriptors) {
    GetEffectDescriptors();
}

TEST_P(AudioEffectTest, DescriptorIdExistAndUnique) {
    auto checker = [&](const std::shared_ptr<IEffect>& effect) {
        Descriptor desc;
        std::vector<Descriptor::Identity> idList;
        EXPECT_IS_OK(effect->getDescriptor(&desc));
        QueryEffects(desc.common.id.type, desc.common.id.uuid, &idList);
        EXPECT_EQ(idList.size(), 1UL);
    };
    ForEachEffect(checker);

    // Check unique with a set
    auto stringHash = [](const Descriptor::Identity& id) {
        return std::hash<std::string>()(id.toString());
    };
    auto vec = GetCompleteEffectIdList();
    std::unordered_set<Descriptor::Identity, decltype(stringHash)> idSet(0, stringHash);
    for (auto it : vec) {
        EXPECT_EQ(idSet.count(it), 0UL);
        idSet.insert(it);
    }
}

/// State testing.
// An effect instance is in INIT state by default after it was created.
TEST_P(AudioEffectTest, InitStateAfterCreation) {
    ExpectState(State::INIT);
}

// An effect instance transfer to INIT state after it was open successfully with IEffect.open().
TEST_P(AudioEffectTest, IdleStateAfterOpen) {
    OpenEffects();
    ExpectState(State::IDLE);
    CloseEffects();
}

// An effect instance is in PROCESSING state after it receive an START command.
TEST_P(AudioEffectTest, ProcessingStateAfterStart) {
    OpenEffects();
    CommandEffects(CommandId::START);
    ExpectState(State::PROCESSING);
    CommandEffects(CommandId::STOP);
    CloseEffects();
}

// An effect instance transfer to IDLE state after Command.Id.STOP in PROCESSING state.
TEST_P(AudioEffectTest, IdleStateAfterStop) {
    OpenEffects();
    CommandEffects(CommandId::START);
    ExpectState(State::PROCESSING);
    CommandEffects(CommandId::STOP);
    ExpectState(State::IDLE);
    CloseEffects();
}

// An effect instance transfer to IDLE state after Command.Id.RESET in PROCESSING state.
TEST_P(AudioEffectTest, IdleStateAfterReset) {
    OpenEffects();
    CommandEffects(CommandId::START);
    ExpectState(State::PROCESSING);
    CommandEffects(CommandId::RESET);
    ExpectState(State::IDLE);
    CloseEffects();
}

// An effect instance transfer to INIT if instance receive a close() call.
TEST_P(AudioEffectTest, InitStateAfterClose) {
    OpenEffects();
    CommandEffects(CommandId::START);
    ExpectState(State::PROCESSING);
    CommandEffects(CommandId::STOP);
    ExpectState(State::IDLE);
    CloseEffects();
    ExpectState(State::INIT);
}

// An effect instance shouldn't accept any command before open.
TEST_P(AudioEffectTest, NoCommandAcceptedBeforeOpen) {
    ExpectState(State::INIT);
    CommandEffectsExpectStatus(CommandId::START, EX_ILLEGAL_STATE);
    CommandEffectsExpectStatus(CommandId::STOP, EX_ILLEGAL_STATE);
    CommandEffectsExpectStatus(CommandId::RESET, EX_ILLEGAL_STATE);
    ExpectState(State::INIT);
}

// No-op when receive STOP command in IDLE state.
TEST_P(AudioEffectTest, StopCommandInIdleStateNoOp) {
    ExpectState(State::INIT);
    OpenEffects();
    ExpectState(State::IDLE);
    CommandEffects(CommandId::STOP);
    ExpectState(State::IDLE);
    CloseEffects();
}

// No-op when receive STOP command in IDLE state.
TEST_P(AudioEffectTest, ResetCommandInIdleStateNoOp) {
    ExpectState(State::INIT);
    OpenEffects();
    ExpectState(State::IDLE);
    CommandEffects(CommandId::RESET);
    ExpectState(State::IDLE);
    CloseEffects();
}

// Repeat START and STOP command.
TEST_P(AudioEffectTest, RepeatStartAndStop) {
    OpenEffects();
    CommandEffects(CommandId::START);
    ExpectState(State::PROCESSING);
    CommandEffects(CommandId::STOP);
    ExpectState(State::IDLE);
    CommandEffects(CommandId::START);
    ExpectState(State::PROCESSING);
    CommandEffects(CommandId::STOP);
    ExpectState(State::IDLE);
    CloseEffects();
}

// Repeat START and RESET command.
TEST_P(AudioEffectTest, RepeatStartAndReset) {
    OpenEffects();
    CommandEffects(CommandId::START);
    ExpectState(State::PROCESSING);
    CommandEffects(CommandId::RESET);
    ExpectState(State::IDLE);
    CommandEffects(CommandId::START);
    ExpectState(State::PROCESSING);
    CommandEffects(CommandId::RESET);
    ExpectState(State::IDLE);
    CloseEffects();
}

// Repeat START and STOP command, try to close at PROCESSING state.
TEST_P(AudioEffectTest, CloseProcessingStateEffects) {
    OpenEffects();
    CommandEffects(CommandId::START);
    ExpectState(State::PROCESSING);
    CommandEffects(CommandId::STOP);
    ExpectState(State::IDLE);
    CommandEffects(CommandId::START);
    ExpectState(State::PROCESSING);
    CloseEffects(EX_ILLEGAL_STATE);
    // cleanup
    CommandEffects(CommandId::STOP);
    ExpectState(State::IDLE);
}

// Expect EX_ILLEGAL_STATE if the effect instance is not in a proper state to be destroyed.
TEST_P(AudioEffectTest, DestroyOpenEffects) {
    // cleanup all effects.
    CloseEffects();
    DestroyEffects();

    // open effects, destroy without close, expect to get EX_ILLEGAL_STATE status.
    CreateEffects();
    OpenEffects();
    DestroyEffects(EX_ILLEGAL_STATE, 1);
    CloseEffects();
}

/// Parameter testing.
// Verify parameters pass in open can be successfully get.
TEST_P(AudioEffectTest, VerifyParametersAfterOpen) {
    OpenEffects();
    VerifyParameters();
    CloseEffects();
}

// Verify parameters pass in set can be successfully get.
TEST_P(AudioEffectTest, SetAndGetParameter) {
    OpenEffects();
    VerifyParameters();
    initParamCommon(1 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */,
                    44100 /* oSampleRate */);
    SetParameter();
    VerifyParameters();
    CloseEffects();
}

// Verify parameters pass in set can be successfully get.
TEST_P(AudioEffectTest, SetAndGetParameterInProcessing) {
    OpenEffects();
    VerifyParameters();
    CommandEffects(CommandId::START);
    ExpectState(State::PROCESSING);
    initParamCommon(1 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */,
                    44100 /* oSampleRate */);
    SetParameter();
    VerifyParameters();
    CommandEffects(CommandId::STOP);
    ExpectState(State::IDLE);
    CloseEffects();
}

// Parameters kept after reset.
TEST_P(AudioEffectTest, ResetAndVerifyParameter) {
    OpenEffects();
    VerifyParameters();
    CommandEffects(CommandId::START);
    ExpectState(State::PROCESSING);
    initParamCommon(1 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */,
                    44100 /* oSampleRate */);
    SetParameter();
    VerifyParameters();
    CommandEffects(CommandId::RESET);
    ExpectState(State::IDLE);
    VerifyParameters();
    CloseEffects();
}

// Multiple instances of same implementation running.
TEST_P(AudioEffectTest, MultipleInstancesRunning) {
    CreateEffects(3);
    ExpectState(State::INIT);
    OpenEffects();
    ExpectState(State::IDLE);
    CommandEffects(CommandId::START);
    ExpectState(State::PROCESSING);
    initParamCommon(1 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */,
                    44100 /* oSampleRate */);
    SetParameter();
    VerifyParameters();
    CommandEffects(CommandId::STOP);
    ExpectState(State::IDLE);
    VerifyParameters();
    CloseEffects();
}

// Send data to effects and expect it to consume by check statusMQ.
TEST_P(AudioEffectTest, ExpectEffectsToConsumeDataInMQ) {
    OpenEffects();
    PrepareInputData(mWriteMQSize);

    CommandEffects(CommandId::START);
    writeToFmq(mWriteMQSize);
    readFromFmq(mWriteMQSize);

    ExpectState(State::PROCESSING);
    CommandEffects(CommandId::STOP);
    // cleanup
    CommandEffects(CommandId::STOP);
    ExpectState(State::IDLE);
    CloseEffects();
}

INSTANTIATE_TEST_SUITE_P(AudioEffectTestTest, AudioEffectTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IFactory::descriptor)),
                         android::PrintInstanceNameToString);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AudioEffectTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
