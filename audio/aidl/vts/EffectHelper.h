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

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <aidl/android/hardware/audio/effect/IEffect.h>
#include <aidl/android/hardware/audio/effect/IFactory.h>
#include <aidl/android/media/audio/common/AudioChannelLayout.h>
#include <aidl/android/media/audio/common/AudioDeviceType.h>
#include <android/binder_auto_utils.h>
#include <fmq/AidlMessageQueue.h>

#include "AudioHalBinderServiceUtil.h"
#include "EffectFactoryHelper.h"
#include "TestUtils.h"

using namespace android;
using aidl::android::hardware::audio::effect::CommandId;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::EffectNullUuid;
using aidl::android::hardware::audio::effect::EffectZeroUuid;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::Parameter;
using aidl::android::hardware::audio::effect::State;
using aidl::android::hardware::common::fmq::SynchronizedReadWrite;
using aidl::android::media::audio::common::AudioChannelLayout;
using aidl::android::media::audio::common::AudioDeviceType;
using aidl::android::media::audio::common::AudioFormatDescription;
using aidl::android::media::audio::common::AudioFormatType;
using aidl::android::media::audio::common::AudioUuid;
using aidl::android::media::audio::common::PcmType;

const AudioFormatDescription DefaultFormat = {
        .type = AudioFormatType::PCM, .pcm = PcmType::INT_16_BIT, .encoding = ""};

class EffectHelper {
  public:
    explicit EffectHelper(const std::string& name) : mFactoryHelper(EffectFactoryHelper(name)) {
        mFactoryHelper.ConnectToFactoryService();
    }

    void OpenEffects(const AudioUuid& type = EffectNullUuid) {
        auto open = [&](const std::shared_ptr<IEffect>& effect) {
            IEffect::OpenEffectReturn ret;
            EXPECT_IS_OK(effect->open(mCommon, mSpecific, &ret));
            EffectParam params;
            params.statusMQ = std::make_unique<StatusMQ>(ret.statusMQ);
            params.inputMQ = std::make_unique<DataMQ>(ret.inputDataMQ);
            params.outputMQ = std::make_unique<DataMQ>(ret.outputDataMQ);
            mEffectParams.push_back(std::move(params));
        };
        EXPECT_NO_FATAL_FAILURE(ForEachEffect(open, type));
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

    void CreateEffectsWithUUID(const AudioUuid& type = EffectNullUuid) {
        ASSERT_NO_FATAL_FAILURE(mFactoryHelper.QueryAndCreateEffects(type));
    }

    void QueryEffects() { ASSERT_NO_FATAL_FAILURE(mFactoryHelper.QueryAndCreateAllEffects()); }

    void DestroyEffects(const binder_status_t status = EX_NONE, const int remaining = 0) {
        ASSERT_NO_FATAL_FAILURE(mFactoryHelper.DestroyEffects(status, remaining));
        mEffectDescriptors.clear();
    }

    void GetEffectDescriptors() {
        auto get = [&](const std::shared_ptr<IEffect>& effect) {
            Descriptor desc;
            EXPECT_IS_OK(effect->getDescriptor(&desc));
            mEffectDescriptors.push_back(std::move(desc));
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

    void QueryEffects(const std::optional<AudioUuid>& in_type,
                      const std::optional<AudioUuid>& in_instance,
                      std::vector<Descriptor::Identity>* _aidl_return) {
        mFactoryHelper.QueryEffects(in_type, in_instance, _aidl_return);
    }

    template <typename Functor>
    void ForEachEffect(Functor functor, const std::optional<AudioUuid>& type = EffectNullUuid) {
        auto effectMap = mFactoryHelper.GetEffectMap();
        for (const auto& it : effectMap) {
            SCOPED_TRACE(it.second.toString());
            if (type != EffectNullUuid && it.second.type != type) continue;
            functor(it.first);
        }
    }

    template <typename Functor>
    void ForEachDescriptor(Functor functor) {
        for (size_t i = 0; i < mEffectDescriptors.size(); i++) {
            SCOPED_TRACE(mEffectDescriptors[i].toString());
            functor(i, mEffectDescriptors[i]);
        }
    }

    static const size_t mWriteMQSize = 0x400;

    enum class IO : char { INPUT = 0, OUTPUT = 1, INOUT = 2 };

    void initParamCommonFormat(IO io = IO::INOUT,
                               const AudioFormatDescription& format = DefaultFormat) {
        if (io == IO::INPUT || io == IO::INOUT) {
            mCommon.input.base.format = format;
        }
        if (io == IO::OUTPUT || io == IO::INOUT) {
            mCommon.output.base.format = format;
        }
    }

    void initParamCommonSampleRate(IO io = IO::INOUT, const int& sampleRate = 48000) {
        if (io == IO::INPUT || io == IO::INOUT) {
            mCommon.input.base.sampleRate = sampleRate;
        }
        if (io == IO::OUTPUT || io == IO::INOUT) {
            mCommon.output.base.sampleRate = sampleRate;
        }
    }

    void initParamCommonFrameCount(IO io = IO::INOUT, const long& frameCount = 48000) {
        if (io == IO::INPUT || io == IO::INOUT) {
            mCommon.input.frameCount = frameCount;
        }
        if (io == IO::OUTPUT || io == IO::INOUT) {
            mCommon.output.frameCount = frameCount;
        }
    }
    void initParamCommon(int session = -1, int ioHandle = -1, int iSampleRate = 48000,
                         int oSampleRate = 48000, long iFrameCount = 0x100,
                         long oFrameCount = 0x100) {
        mCommon.session = session;
        mCommon.ioHandle = ioHandle;

        auto& input = mCommon.input;
        auto& output = mCommon.output;
        input.base.sampleRate = iSampleRate;
        input.base.channelMask = mInputChannelLayout;
        input.frameCount = iFrameCount;
        output.base.sampleRate = oSampleRate;
        output.base.channelMask = mOutputChannelLayout;
        output.base.format = DefaultFormat;
        output.frameCount = oFrameCount;
        inputFrameSize = android::hardware::audio::common::getFrameSizeInBytes(
                input.base.format, input.base.channelMask);
        outputFrameSize = android::hardware::audio::common::getFrameSizeInBytes(
                output.base.format, output.base.channelMask);
    }

    void setSpecific(Parameter::Specific& specific) { mSpecific = specific; }

    // usually this function only call once.
    void PrepareInputData(size_t s = mWriteMQSize) {
        size_t maxInputSize = s;
        for (auto& it : mEffectParams) {
            auto& mq = it.inputMQ;
            EXPECT_NE(nullptr, mq);
            EXPECT_TRUE(mq->isValid());
            const size_t bytesToWrite = mq->availableToWrite();
            EXPECT_EQ(inputFrameSize * mCommon.input.frameCount, bytesToWrite);
            EXPECT_NE(0UL, bytesToWrite);
            EXPECT_TRUE(s <= bytesToWrite);
            maxInputSize = std::max(maxInputSize, bytesToWrite);
        }
        mInputBuffer.resize(maxInputSize);
        std::fill(mInputBuffer.begin(), mInputBuffer.end(), 0x5a);
    }

    void writeToFmq(size_t s = mWriteMQSize) {
        for (auto& it : mEffectParams) {
            auto& mq = it.inputMQ;
            EXPECT_NE(nullptr, mq);
            const size_t bytesToWrite = mq->availableToWrite();
            EXPECT_NE(0Ul, bytesToWrite);
            EXPECT_TRUE(s <= bytesToWrite);
            EXPECT_TRUE(mq->write(mInputBuffer.data(), s));
        }
    }

    void readFromFmq(size_t expectSize = mWriteMQSize) {
        for (auto& it : mEffectParams) {
            IEffect::Status status{};
            auto& statusMq = it.statusMQ;
            EXPECT_NE(nullptr, statusMq);
            EXPECT_TRUE(statusMq->readBlocking(&status, 1));
            EXPECT_EQ(STATUS_OK, status.status);
            EXPECT_EQ(expectSize, (unsigned)status.fmqByteProduced);

            auto& outputMq = it.outputMQ;
            EXPECT_NE(nullptr, outputMq);
            EXPECT_EQ(expectSize, outputMq->availableToRead());
        }
    }

    void setInputChannelLayout(AudioChannelLayout input) { mInputChannelLayout = input; }
    void setOutputChannelLayout(AudioChannelLayout output) { mOutputChannelLayout = output; }
    const std::vector<Descriptor::Identity>& GetCompleteEffectIdList() const {
        return mFactoryHelper.GetCompleteEffectIdList();
    }
    const std::vector<Descriptor>& getDescriptorVec() const { return mEffectDescriptors; }

  private:
    EffectFactoryHelper mFactoryHelper;

    AudioChannelLayout mInputChannelLayout =
            AudioChannelLayout::make<AudioChannelLayout::layoutMask>(
                    AudioChannelLayout::LAYOUT_STEREO);
    AudioChannelLayout mOutputChannelLayout =
            AudioChannelLayout::make<AudioChannelLayout::layoutMask>(
                    AudioChannelLayout::LAYOUT_STEREO);

    Parameter::Common mCommon;
    Parameter::Specific mSpecific;

    size_t inputFrameSize, outputFrameSize;
    std::vector<int8_t> mInputBuffer;  // reuse same buffer for all effects testing

    typedef ::android::AidlMessageQueue<
            IEffect::Status, ::aidl::android::hardware::common::fmq::SynchronizedReadWrite>
            StatusMQ;
    typedef ::android::AidlMessageQueue<
            int8_t, ::aidl::android::hardware::common::fmq::SynchronizedReadWrite>
            DataMQ;

    class EffectParam {
      public:
        std::unique_ptr<StatusMQ> statusMQ;
        std::unique_ptr<DataMQ> inputMQ;
        std::unique_ptr<DataMQ> outputMQ;
    };
    std::vector<EffectParam> mEffectParams;
    std::vector<Descriptor> mEffectDescriptors;
};
