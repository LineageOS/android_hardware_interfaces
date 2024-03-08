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

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/audio/effect/IEffect.h>
#include <aidl/android/hardware/audio/effect/IFactory.h>
#include <android-base/logging.h>
#include <android/binder_interface_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <fmq/AidlMessageQueue.h>

#include "AudioHalBinderServiceUtil.h"
#include "EffectFactoryHelper.h"
#include "EffectHelper.h"
#include "TestUtils.h"

using namespace android;

using ndk::ScopedAStatus;

using aidl::android::hardware::audio::effect::CommandId;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::Flags;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::IFactory;
using aidl::android::hardware::audio::effect::Parameter;
using aidl::android::hardware::audio::effect::State;
using aidl::android::media::audio::common::AudioDeviceDescription;
using aidl::android::media::audio::common::AudioDeviceType;
using aidl::android::media::audio::common::AudioMode;
using aidl::android::media::audio::common::AudioSource;
using android::hardware::audio::common::testing::detail::TestExecutionTracer;

enum ParamName { PARAM_INSTANCE_NAME };
using EffectTestParam = std::tuple<std::pair<std::shared_ptr<IFactory>, Descriptor>>;

class AudioEffectTest : public testing::TestWithParam<EffectTestParam>, public EffectHelper {
  public:
    AudioEffectTest() {
        std::tie(mFactory, mDescriptor) = std::get<PARAM_INSTANCE_NAME>(GetParam());
    }

    void SetUp() override {}

    void TearDown() override {
        // Do the cleanup for every test case
        if (mEffect) {
            ASSERT_NO_FATAL_FAILURE(commandIgnoreRet(mEffect, CommandId::STOP));
            ASSERT_NO_FATAL_FAILURE(closeIgnoreRet(mEffect));
            ASSERT_NO_FATAL_FAILURE(destroyIgnoreRet(mFactory, mEffect));
            mEffect.reset();
        }
    }

    static const long kInputFrameCount = 0x100, kOutputFrameCount = 0x100;
    std::shared_ptr<IFactory> mFactory;
    std::shared_ptr<IEffect> mEffect;
    Descriptor mDescriptor;

    void setAndGetParameter(Parameter::Id id, const Parameter& set) {
        Parameter get;
        EXPECT_IS_OK(mEffect->setParameter(set));
        EXPECT_IS_OK(mEffect->getParameter(id, &get));
        EXPECT_EQ(set, get) << set.toString() << "\n vs \n" << get.toString();
    }
};

class AudioEffectDataPathTest : public AudioEffectTest {
  public:
    void SetUp() override {
        AudioEffectTest::SetUp();
        SKIP_TEST_IF_DATA_UNSUPPORTED(mDescriptor.common.flags);
    }
};

TEST_P(AudioEffectTest, SetupAndTearDown) {
    // Intentionally empty test body.
}

TEST_P(AudioEffectTest, CreateAndDestroy) {
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

TEST_P(AudioEffectTest, OpenAndClose) {
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(open(mEffect));
    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

TEST_P(AudioEffectTest, CloseUnopenedEffect) {
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

TEST_P(AudioEffectTest, DoubleOpenAndClose) {
    std::shared_ptr<IEffect> effect1, effect2;
    ASSERT_NO_FATAL_FAILURE(create(mFactory, effect1, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(create(mFactory, effect2, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(open(effect1));
    ASSERT_NO_FATAL_FAILURE(open(effect2, 1 /* session */));
    ASSERT_NO_FATAL_FAILURE(close(effect1));
    ASSERT_NO_FATAL_FAILURE(close(effect2));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, effect1));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, effect2));
}

TEST_P(AudioEffectTest, TripleOpenAndClose) {
    std::shared_ptr<IEffect> effect1, effect2, effect3;
    ASSERT_NO_FATAL_FAILURE(create(mFactory, effect1, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(create(mFactory, effect2, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(create(mFactory, effect3, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(open(effect1));
    ASSERT_NO_FATAL_FAILURE(open(effect2, 1 /* session */));
    ASSERT_NO_FATAL_FAILURE(open(effect3, 2 /* session */));
    ASSERT_NO_FATAL_FAILURE(close(effect1));
    ASSERT_NO_FATAL_FAILURE(close(effect2));
    ASSERT_NO_FATAL_FAILURE(close(effect3));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, effect1));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, effect2));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, effect3));
}

TEST_P(AudioEffectTest, GetDescritorBeforeOpen) {
    Descriptor desc;
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(getDescriptor(mEffect, desc));
    EXPECT_EQ(mDescriptor.common.id.type, desc.common.id.type);
    EXPECT_EQ(mDescriptor.common.id.uuid, desc.common.id.uuid);
    EXPECT_EQ(mDescriptor.common.name, desc.common.name);
    EXPECT_EQ(mDescriptor.common.implementor, desc.common.implementor);
    // Effect implementation Must fill in implementor and name
    EXPECT_NE("", desc.common.name);
    EXPECT_NE("", desc.common.implementor);
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

TEST_P(AudioEffectTest, GetDescritorAfterOpen) {
    Descriptor beforeOpen, afterOpen, afterClose;
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(getDescriptor(mEffect, beforeOpen));
    ASSERT_NO_FATAL_FAILURE(open(mEffect));
    ASSERT_NO_FATAL_FAILURE(getDescriptor(mEffect, afterOpen));
    EXPECT_EQ(beforeOpen.toString(), afterOpen.toString()) << "\n"
                                                           << beforeOpen.toString() << "\n"
                                                           << afterOpen.toString();
    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(getDescriptor(mEffect, afterClose));
    EXPECT_EQ(beforeOpen.toString(), afterClose.toString()) << "\n"
                                                            << beforeOpen.toString() << "\n"
                                                            << afterClose.toString();
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

TEST_P(AudioEffectTest, DescriptorExistAndUnique) {
    Descriptor desc;

    auto descList = EffectFactoryHelper::getAllEffectDescriptors(IFactory::descriptor);
    std::set<Descriptor::Identity> idSet;
    for (const auto& it : descList) {
        auto& id = it.second.common.id;
        EXPECT_EQ(0ul, idSet.count(id));
        idSet.insert(id);
    }

    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(getDescriptor(mEffect, desc));
    int uuidCount = std::count_if(idSet.begin(), idSet.end(), [&](const auto& id) {
        return id.uuid == desc.common.id.uuid && id.type == desc.common.id.type;
    });

    EXPECT_EQ(1, uuidCount);
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

/// State testing.
// An effect instance is in INIT state by default after it was created.
TEST_P(AudioEffectTest, InitStateAfterCreation) {
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::INIT));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

// An effect instance transfer to IDLE state after IEffect.ASSERT_NO_FATAL_FAILURE(open().
TEST_P(AudioEffectTest, IdleStateAfterOpen) {
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(open(mEffect));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

// An effect instance is in PROCESSING state after it receive an START command.
TEST_P(AudioEffectTest, ProcessingStateAfterStart) {
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::INIT));
    ASSERT_NO_FATAL_FAILURE(open(mEffect));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::STOP));
    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

// An effect instance transfer to IDLE state after Command.Id.STOP in PROCESSING state.
TEST_P(AudioEffectTest, IdleStateAfterStop) {
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(open(mEffect));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::STOP));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

// An effect instance transfer to IDLE state after Command.Id.RESET in PROCESSING state.
TEST_P(AudioEffectTest, IdleStateAfterReset) {
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(open(mEffect));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::RESET));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

// An effect instance transfer to INIT after IEffect.ASSERT_NO_FATAL_FAILURE(close().
TEST_P(AudioEffectTest, InitStateAfterClose) {
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(open(mEffect));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::STOP));
    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::INIT));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

// An effect instance shouldn't accept any command before open.
TEST_P(AudioEffectTest, NoCommandAcceptedBeforeOpen) {
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START, EX_ILLEGAL_STATE));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::STOP, EX_ILLEGAL_STATE));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::RESET, EX_ILLEGAL_STATE));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

// No-op when receive STOP command in IDLE state.
TEST_P(AudioEffectTest, StopCommandInIdleStateNoOp) {
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(open(mEffect));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::STOP));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

// No-op when receive RESET command in IDLE state.
TEST_P(AudioEffectTest, ResetCommandInIdleStateNoOp) {
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(open(mEffect));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::RESET));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

// Repeat START and STOP command.
TEST_P(AudioEffectTest, RepeatStartAndStop) {
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(open(mEffect));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::STOP));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));

    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::STOP));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

// Repeat START and RESET command.
TEST_P(AudioEffectTest, RepeatStartAndReset) {
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(open(mEffect));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::RESET));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));

    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::RESET));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

// Try to close an effect instance at PROCESSING state.
TEST_P(AudioEffectTest, CloseProcessingStateEffects) {
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(open(mEffect));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));

    ASSERT_NO_FATAL_FAILURE(close(mEffect, EX_ILLEGAL_STATE));

    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::STOP));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

// Expect EX_ILLEGAL_STATE if the effect instance is not in a proper state to be destroyed.
TEST_P(AudioEffectTest, DestroyOpenEffects) {
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(open(mEffect));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));

    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect, EX_ILLEGAL_STATE));

    // cleanup
    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

// Expect EX_ILLEGAL_STATE if the effect instance is not in a proper state to be destroyed.
TEST_P(AudioEffectTest, DestroyProcessingEffects) {
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(open(mEffect));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));

    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect, EX_ILLEGAL_STATE));

    // cleanup
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::STOP));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

TEST_P(AudioEffectTest, NormalSequenceStates) {
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::INIT));
    ASSERT_NO_FATAL_FAILURE(open(mEffect));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::STOP));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::RESET));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

/// Parameter testing.
// Verify parameters pass in open can be successfully get.
TEST_P(AudioEffectTest, VerifyCommonParametersAfterOpen) {
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));

    Parameter::Common common = EffectHelper::createParamCommon();
    IEffect::OpenEffectReturn ret;
    ASSERT_NO_FATAL_FAILURE(open(mEffect, common, std::nullopt /* specific */, &ret, EX_NONE));

    Parameter get = Parameter(), expect = Parameter();
    expect.set<Parameter::common>(common);
    Parameter::Id id;
    id.set<Parameter::Id::commonTag>(Parameter::common);
    EXPECT_IS_OK(mEffect->getParameter(id, &get));
    EXPECT_EQ(expect, get) << expect.toString() << "\n vs \n" << get.toString();

    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

// Verify parameters pass in set can be successfully get.
TEST_P(AudioEffectTest, SetAndGetCommonParameter) {
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(open(mEffect));

    Parameter::Common common = EffectHelper::createParamCommon(
            0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */);
    Parameter::Id id = Parameter::Id::make<Parameter::Id::commonTag>(Parameter::common);
    ASSERT_NO_FATAL_FAILURE(setAndGetParameter(id, Parameter::make<Parameter::common>(common)));

    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

// Verify parameters set and get in PROCESSING state.
TEST_P(AudioEffectTest, SetAndGetParameterInProcessing) {
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(open(mEffect));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));

    Parameter::Common common = EffectHelper::createParamCommon(
            0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */);
    Parameter::Id id = Parameter::Id::make<Parameter::Id::commonTag>(Parameter::common);
    ASSERT_NO_FATAL_FAILURE(setAndGetParameter(id, Parameter::make<Parameter::common>(common)));

    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::STOP));
    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

// Verify parameters set and get in IDLE state.
TEST_P(AudioEffectTest, SetAndGetParameterInIdle) {
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(open(mEffect));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::STOP));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));

    Parameter::Common common = EffectHelper::createParamCommon(
            0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */);
    Parameter::Id id = Parameter::Id::make<Parameter::Id::commonTag>(Parameter::common);
    ASSERT_NO_FATAL_FAILURE(setAndGetParameter(id, Parameter::make<Parameter::common>(common)));

    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

// Verify Parameters kept after stop.
TEST_P(AudioEffectTest, SetAndGetParameterAfterStop) {
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(open(mEffect));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));

    Parameter::Common common = EffectHelper::createParamCommon(
            0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */);
    Parameter::Id id = Parameter::Id::make<Parameter::Id::commonTag>(Parameter::common);
    ASSERT_NO_FATAL_FAILURE(setAndGetParameter(id, Parameter::make<Parameter::common>(common)));

    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::STOP));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

// Verify Parameters kept after reset.
TEST_P(AudioEffectTest, SetAndGetParameterAfterReset) {
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(open(mEffect));

    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));

    Parameter::Common common = EffectHelper::createParamCommon(
            0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */);
    Parameter::Id id = Parameter::Id::make<Parameter::Id::commonTag>(Parameter::common);
    ASSERT_NO_FATAL_FAILURE(setAndGetParameter(id, Parameter::make<Parameter::common>(common)));

    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::RESET));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));

    ASSERT_NO_FATAL_FAILURE(setAndGetParameter(id, Parameter::make<Parameter::common>(common)));

    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::STOP));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

// Set and get AudioDeviceDescription in Parameter
TEST_P(AudioEffectTest, SetAndGetParameterDeviceDescription) {
    if (!mDescriptor.common.flags.deviceIndication) {
        GTEST_SKIP() << "Skipping test as effect does not support deviceIndication"
                     << mDescriptor.common.flags.toString();
    }

    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(open(mEffect));

    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));

    std::vector<AudioDeviceDescription> deviceDescs = {
            {.type = AudioDeviceType::IN_DEFAULT,
             .connection = AudioDeviceDescription::CONNECTION_ANALOG},
            {.type = AudioDeviceType::IN_DEVICE,
             .connection = AudioDeviceDescription::CONNECTION_BT_A2DP}};
    Parameter::Id id = Parameter::Id::make<Parameter::Id::commonTag>(Parameter::deviceDescription);
    ASSERT_NO_FATAL_FAILURE(
            setAndGetParameter(id, Parameter::make<Parameter::deviceDescription>(deviceDescs)));

    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::STOP));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

// Set and get AudioMode in Parameter
TEST_P(AudioEffectTest, SetAndGetParameterAudioMode) {
    if (!mDescriptor.common.flags.audioModeIndication) {
        GTEST_SKIP() << "Skipping test as effect does not support audioModeIndication"
                     << mDescriptor.common.flags.toString();
    }

    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(open(mEffect));

    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));

    Parameter::Id id = Parameter::Id::make<Parameter::Id::commonTag>(Parameter::mode);
    ASSERT_NO_FATAL_FAILURE(
            setAndGetParameter(id, Parameter::make<Parameter::mode>(AudioMode::NORMAL)));
    ASSERT_NO_FATAL_FAILURE(
            setAndGetParameter(id, Parameter::make<Parameter::mode>(AudioMode::IN_COMMUNICATION)));

    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::STOP));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

// Set and get AudioSource in Parameter
TEST_P(AudioEffectTest, SetAndGetParameterAudioSource) {
    if (!mDescriptor.common.flags.audioSourceIndication) {
        GTEST_SKIP() << "Skipping test as effect does not support audioSourceIndication"
                     << mDescriptor.common.flags.toString();
    }

    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(open(mEffect));

    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));

    Parameter::Id id = Parameter::Id::make<Parameter::Id::commonTag>(Parameter::source);
    ASSERT_NO_FATAL_FAILURE(
            setAndGetParameter(id, Parameter::make<Parameter::source>(AudioSource::DEFAULT)));
    ASSERT_NO_FATAL_FAILURE(setAndGetParameter(
            id, Parameter::make<Parameter::source>(AudioSource::VOICE_RECOGNITION)));

    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::STOP));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

// Set and get VolumeStereo in Parameter
TEST_P(AudioEffectTest, SetAndGetParameterVolume) {
    if (mDescriptor.common.flags.volume == Flags::Volume::NONE) {
        GTEST_SKIP() << "Skipping test as effect does not support volume"
                     << mDescriptor.common.flags.toString();
    }

    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(open(mEffect));

    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));

    Parameter::Id id = Parameter::Id::make<Parameter::Id::commonTag>(Parameter::volumeStereo);
    Parameter::VolumeStereo volume = {.left = 10.0, .right = 10.0};
    if (mDescriptor.common.flags.volume == Flags::Volume::CTRL) {
        Parameter get;
        EXPECT_IS_OK(mEffect->setParameter(volume));
        EXPECT_IS_OK(mEffect->getParameter(id, &get));
    } else {
        ASSERT_NO_FATAL_FAILURE(
                setAndGetParameter(id, Parameter::make<Parameter::volumeStereo>(volume)));
    }

    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::STOP));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

/// Data processing test
// Send data to effects and expect it to be consumed by checking statusMQ.
// Effects exposing bypass flags or operating in offload mode will be skipped.
TEST_P(AudioEffectDataPathTest, ConsumeDataInProcessingState) {
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));

    Parameter::Common common = EffectHelper::createParamCommon(
            0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */,
            kInputFrameCount /* iFrameCount */, kOutputFrameCount /* oFrameCount */);
    IEffect::OpenEffectReturn ret;
    ASSERT_NO_FATAL_FAILURE(open(mEffect, common, std::nullopt /* specific */, &ret, EX_NONE));
    auto statusMQ = std::make_unique<EffectHelper::StatusMQ>(ret.statusMQ);
    ASSERT_TRUE(statusMQ->isValid());
    auto inputMQ = std::make_unique<EffectHelper::DataMQ>(ret.inputDataMQ);
    ASSERT_TRUE(inputMQ->isValid());
    auto outputMQ = std::make_unique<EffectHelper::DataMQ>(ret.outputDataMQ);
    ASSERT_TRUE(outputMQ->isValid());

    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));

    std::vector<float> buffer;
    EXPECT_NO_FATAL_FAILURE(EffectHelper::allocateInputData(common, inputMQ, buffer));
    EXPECT_NO_FATAL_FAILURE(EffectHelper::writeToFmq(statusMQ, inputMQ, buffer));
    EXPECT_NO_FATAL_FAILURE(
            EffectHelper::readFromFmq(statusMQ, 1, outputMQ, buffer.size(), buffer));

    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::STOP));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    EXPECT_NO_FATAL_FAILURE(EffectHelper::readFromFmq(statusMQ, 0, outputMQ, 0, buffer));

    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

// Send data to effects and expect it to be consumed after effect restart.
// Effects exposing bypass flags or operating in offload mode will be skipped.
TEST_P(AudioEffectDataPathTest, ConsumeDataAfterRestart) {
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));

    Parameter::Common common = EffectHelper::createParamCommon(
            0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */,
            kInputFrameCount /* iFrameCount */, kOutputFrameCount /* oFrameCount */);
    IEffect::OpenEffectReturn ret;
    ASSERT_NO_FATAL_FAILURE(open(mEffect, common, std::nullopt /* specific */, &ret, EX_NONE));
    auto statusMQ = std::make_unique<EffectHelper::StatusMQ>(ret.statusMQ);
    ASSERT_TRUE(statusMQ->isValid());
    auto inputMQ = std::make_unique<EffectHelper::DataMQ>(ret.inputDataMQ);
    ASSERT_TRUE(inputMQ->isValid());
    auto outputMQ = std::make_unique<EffectHelper::DataMQ>(ret.outputDataMQ);
    ASSERT_TRUE(outputMQ->isValid());

    std::vector<float> buffer;
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::STOP));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    EXPECT_NO_FATAL_FAILURE(
            EffectHelper::readFromFmq(statusMQ, 0, outputMQ, buffer.size(), buffer));
    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));

    EXPECT_NO_FATAL_FAILURE(EffectHelper::allocateInputData(common, inputMQ, buffer));
    EXPECT_NO_FATAL_FAILURE(EffectHelper::writeToFmq(statusMQ, inputMQ, buffer));
    EXPECT_NO_FATAL_FAILURE(
            EffectHelper::readFromFmq(statusMQ, 1, outputMQ, buffer.size(), buffer));

    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::STOP));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    EXPECT_NO_FATAL_FAILURE(EffectHelper::readFromFmq(statusMQ, 0, outputMQ, 0, buffer));

    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

// Send data to IDLE effects and expect it to be consumed after effect start.
// Effects exposing bypass flags or operating in offload mode will be skipped.
TEST_P(AudioEffectDataPathTest, SendDataAtIdleAndConsumeDataInProcessing) {
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));

    Parameter::Common common = EffectHelper::createParamCommon(
            0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */,
            kInputFrameCount /* iFrameCount */, kOutputFrameCount /* oFrameCount */);
    IEffect::OpenEffectReturn ret;
    ASSERT_NO_FATAL_FAILURE(open(mEffect, common, std::nullopt /* specific */, &ret, EX_NONE));
    auto statusMQ = std::make_unique<EffectHelper::StatusMQ>(ret.statusMQ);
    ASSERT_TRUE(statusMQ->isValid());
    auto inputMQ = std::make_unique<EffectHelper::DataMQ>(ret.inputDataMQ);
    ASSERT_TRUE(inputMQ->isValid());
    auto outputMQ = std::make_unique<EffectHelper::DataMQ>(ret.outputDataMQ);
    ASSERT_TRUE(outputMQ->isValid());

    std::vector<float> buffer;
    EXPECT_NO_FATAL_FAILURE(EffectHelper::allocateInputData(common, inputMQ, buffer));
    EXPECT_NO_FATAL_FAILURE(EffectHelper::writeToFmq(statusMQ, inputMQ, buffer));

    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));

    EXPECT_NO_FATAL_FAILURE(
            EffectHelper::readFromFmq(statusMQ, 1, outputMQ, buffer.size(), buffer));

    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::STOP));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));

    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

// Send data multiple times.
// Effects exposing bypass flags or operating in offload mode will be skipped.
TEST_P(AudioEffectDataPathTest, ProcessDataMultipleTimes) {
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));

    Parameter::Common common = EffectHelper::createParamCommon(
            0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */,
            kInputFrameCount /* iFrameCount */, kOutputFrameCount /* oFrameCount */);
    IEffect::OpenEffectReturn ret;
    ASSERT_NO_FATAL_FAILURE(open(mEffect, common, std::nullopt /* specific */, &ret, EX_NONE));
    auto statusMQ = std::make_unique<EffectHelper::StatusMQ>(ret.statusMQ);
    ASSERT_TRUE(statusMQ->isValid());
    auto inputMQ = std::make_unique<EffectHelper::DataMQ>(ret.inputDataMQ);
    ASSERT_TRUE(inputMQ->isValid());
    auto outputMQ = std::make_unique<EffectHelper::DataMQ>(ret.outputDataMQ);
    ASSERT_TRUE(outputMQ->isValid());

    std::vector<float> buffer;
    EXPECT_NO_FATAL_FAILURE(EffectHelper::allocateInputData(common, inputMQ, buffer));
    EXPECT_NO_FATAL_FAILURE(EffectHelper::writeToFmq(statusMQ, inputMQ, buffer));
    EXPECT_NO_FATAL_FAILURE(EffectHelper::readFromFmq(statusMQ, 0, outputMQ, 0, buffer));

    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));

    EXPECT_NO_FATAL_FAILURE(
            EffectHelper::readFromFmq(statusMQ, 1, outputMQ, buffer.size(), buffer));

    EXPECT_NO_FATAL_FAILURE(EffectHelper::writeToFmq(statusMQ, inputMQ, buffer));
    EXPECT_NO_FATAL_FAILURE(
            EffectHelper::readFromFmq(statusMQ, 1, outputMQ, buffer.size(), buffer));

    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::STOP));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    EXPECT_NO_FATAL_FAILURE(EffectHelper::readFromFmq(statusMQ, 0, outputMQ, 0, buffer));

    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

// Send data to processing state effects, stop, and restart.
// Effects exposing bypass flags or operating in offload mode will be skipped.
TEST_P(AudioEffectDataPathTest, ConsumeDataAndRestart) {
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));

    Parameter::Common common = EffectHelper::createParamCommon(
            0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */,
            kInputFrameCount /* iFrameCount */, kOutputFrameCount /* oFrameCount */);
    IEffect::OpenEffectReturn ret;
    ASSERT_NO_FATAL_FAILURE(open(mEffect, common, std::nullopt /* specific */, &ret, EX_NONE));
    auto statusMQ = std::make_unique<EffectHelper::StatusMQ>(ret.statusMQ);
    ASSERT_TRUE(statusMQ->isValid());
    auto inputMQ = std::make_unique<EffectHelper::DataMQ>(ret.inputDataMQ);
    ASSERT_TRUE(inputMQ->isValid());
    auto outputMQ = std::make_unique<EffectHelper::DataMQ>(ret.outputDataMQ);
    ASSERT_TRUE(outputMQ->isValid());

    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));
    std::vector<float> buffer;
    EXPECT_NO_FATAL_FAILURE(EffectHelper::allocateInputData(common, inputMQ, buffer));
    EXPECT_NO_FATAL_FAILURE(EffectHelper::writeToFmq(statusMQ, inputMQ, buffer));
    EXPECT_NO_FATAL_FAILURE(
            EffectHelper::readFromFmq(statusMQ, 1, outputMQ, buffer.size(), buffer));

    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::STOP));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    EXPECT_NO_FATAL_FAILURE(EffectHelper::writeToFmq(statusMQ, inputMQ, buffer));
    EXPECT_NO_FATAL_FAILURE(EffectHelper::readFromFmq(statusMQ, 0, outputMQ, 0, buffer));

    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));
    EXPECT_NO_FATAL_FAILURE(
            EffectHelper::readFromFmq(statusMQ, 1, outputMQ, buffer.size(), buffer));

    ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::STOP));
    ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));

    ASSERT_NO_FATAL_FAILURE(close(mEffect));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

// Send data to closed effects and expect it not be consumed.
// Effects exposing bypass flags or operating in offload mode will be skipped.
TEST_P(AudioEffectDataPathTest, NotConsumeDataByClosedEffect) {
    ASSERT_NO_FATAL_FAILURE(create(mFactory, mEffect, mDescriptor));

    Parameter::Common common = EffectHelper::createParamCommon(
            0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */,
            kInputFrameCount /* iFrameCount */, kOutputFrameCount /* oFrameCount */);
    IEffect::OpenEffectReturn ret;
    ASSERT_NO_FATAL_FAILURE(open(mEffect, common, std::nullopt /* specific */, &ret, EX_NONE));
    ASSERT_NO_FATAL_FAILURE(close(mEffect));

    auto statusMQ = std::make_unique<EffectHelper::StatusMQ>(ret.statusMQ);
    ASSERT_TRUE(statusMQ->isValid());
    auto inputMQ = std::make_unique<EffectHelper::DataMQ>(ret.inputDataMQ);
    ASSERT_TRUE(inputMQ->isValid());
    auto outputMQ = std::make_unique<EffectHelper::DataMQ>(ret.outputDataMQ);
    ASSERT_TRUE(outputMQ->isValid());

    std::vector<float> buffer;
    EXPECT_NO_FATAL_FAILURE(EffectHelper::allocateInputData(common, inputMQ, buffer));
    EXPECT_NO_FATAL_FAILURE(EffectHelper::writeToFmq(statusMQ, inputMQ, buffer));
    EXPECT_NO_FATAL_FAILURE(EffectHelper::readFromFmq(statusMQ, 0, outputMQ, 0, buffer));

    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, mEffect));
}

// Send data to multiple effects.
// Effects exposing bypass flags or operating in offload mode will be skipped.
TEST_P(AudioEffectDataPathTest, ConsumeDataMultipleEffects) {
    std::shared_ptr<IEffect> effect1, effect2;
    ASSERT_NO_FATAL_FAILURE(create(mFactory, effect1, mDescriptor));
    ASSERT_NO_FATAL_FAILURE(create(mFactory, effect2, mDescriptor));

    Parameter::Common common1 = EffectHelper::createParamCommon(
            0 /* session */, 1 /* ioHandle */, 44100 /* iSampleRate */, 44100 /* oSampleRate */,
            kInputFrameCount /* iFrameCount */, kOutputFrameCount /* oFrameCount */);
    Parameter::Common common2 = EffectHelper::createParamCommon(
            1 /* session */, 1 /* ioHandle */, 48000 /* iSampleRate */, 48000 /* oSampleRate */,
            2 * kInputFrameCount /* iFrameCount */, 2 * kOutputFrameCount /* oFrameCount */);
    IEffect::OpenEffectReturn ret1, ret2;
    ASSERT_NO_FATAL_FAILURE(open(effect1, common1, std::nullopt /* specific */, &ret1, EX_NONE));
    ASSERT_NO_FATAL_FAILURE(open(effect2, common2, std::nullopt /* specific */, &ret2, EX_NONE));
    ASSERT_NO_FATAL_FAILURE(command(effect1, CommandId::START));
    ASSERT_NO_FATAL_FAILURE(expectState(effect1, State::PROCESSING));
    ASSERT_NO_FATAL_FAILURE(command(effect2, CommandId::START));
    ASSERT_NO_FATAL_FAILURE(expectState(effect2, State::PROCESSING));

    auto statusMQ1 = std::make_unique<EffectHelper::StatusMQ>(ret1.statusMQ);
    ASSERT_TRUE(statusMQ1->isValid());
    auto inputMQ1 = std::make_unique<EffectHelper::DataMQ>(ret1.inputDataMQ);
    ASSERT_TRUE(inputMQ1->isValid());
    auto outputMQ1 = std::make_unique<EffectHelper::DataMQ>(ret1.outputDataMQ);
    ASSERT_TRUE(outputMQ1->isValid());

    std::vector<float> buffer1, buffer2;
    EXPECT_NO_FATAL_FAILURE(EffectHelper::allocateInputData(common1, inputMQ1, buffer1));
    EXPECT_NO_FATAL_FAILURE(EffectHelper::writeToFmq(statusMQ1, inputMQ1, buffer1));
    EXPECT_NO_FATAL_FAILURE(
            EffectHelper::readFromFmq(statusMQ1, 1, outputMQ1, buffer1.size(), buffer1));

    auto statusMQ2 = std::make_unique<EffectHelper::StatusMQ>(ret2.statusMQ);
    ASSERT_TRUE(statusMQ2->isValid());
    auto inputMQ2 = std::make_unique<EffectHelper::DataMQ>(ret2.inputDataMQ);
    ASSERT_TRUE(inputMQ2->isValid());
    auto outputMQ2 = std::make_unique<EffectHelper::DataMQ>(ret2.outputDataMQ);
    ASSERT_TRUE(outputMQ2->isValid());
    EXPECT_NO_FATAL_FAILURE(EffectHelper::allocateInputData(common2, inputMQ2, buffer2));
    EXPECT_NO_FATAL_FAILURE(EffectHelper::writeToFmq(statusMQ2, inputMQ2, buffer2));
    EXPECT_NO_FATAL_FAILURE(
            EffectHelper::readFromFmq(statusMQ2, 1, outputMQ2, buffer2.size(), buffer2));

    ASSERT_NO_FATAL_FAILURE(command(effect1, CommandId::STOP));
    ASSERT_NO_FATAL_FAILURE(expectState(effect1, State::IDLE));
    ASSERT_NO_FATAL_FAILURE(close(effect1));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, effect1));

    ASSERT_NO_FATAL_FAILURE(command(effect2, CommandId::STOP));
    ASSERT_NO_FATAL_FAILURE(expectState(effect2, State::IDLE));
    ASSERT_NO_FATAL_FAILURE(close(effect2));
    ASSERT_NO_FATAL_FAILURE(destroy(mFactory, effect2));
}

INSTANTIATE_TEST_SUITE_P(
        SingleEffectInstanceTest, AudioEffectTest,
        ::testing::Combine(testing::ValuesIn(
                EffectFactoryHelper::getAllEffectDescriptors(IFactory::descriptor))),
        [](const testing::TestParamInfo<AudioEffectTest::ParamType>& info) {
            auto descriptor = std::get<PARAM_INSTANCE_NAME>(info.param).second;
            std::string name = getPrefix(descriptor);
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AudioEffectTest);

INSTANTIATE_TEST_SUITE_P(
        SingleEffectInstanceTest, AudioEffectDataPathTest,
        ::testing::Combine(testing::ValuesIn(
                EffectFactoryHelper::getAllEffectDescriptors(IFactory::descriptor))),
        [](const testing::TestParamInfo<AudioEffectDataPathTest::ParamType>& info) {
            auto descriptor = std::get<PARAM_INSTANCE_NAME>(info.param).second;
            std::string name = getPrefix(descriptor);
            std::replace_if(
                    name.begin(), name.end(), [](const char c) { return !std::isalnum(c); }, '_');
            return name;
        });

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(AudioEffectDataPathTest);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::UnitTest::GetInstance()->listeners().Append(new TestExecutionTracer());
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
