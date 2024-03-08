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

#include <algorithm>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <Utils.h>
#include <aidl/android/hardware/audio/effect/IEffect.h>
#include <aidl/android/hardware/audio/effect/IFactory.h>
#include <aidl/android/media/audio/common/AudioChannelLayout.h>
#include <android/binder_auto_utils.h>
#include <fmq/AidlMessageQueue.h>
#include <gtest/gtest.h>
#include <system/audio_aidl_utils.h>
#include <system/audio_effects/aidl_effects_utils.h>
#include <system/audio_effects/effect_uuid.h>

#include "AudioHalBinderServiceUtil.h"
#include "EffectFactoryHelper.h"
#include "TestUtils.h"

using namespace android;
using aidl::android::hardware::audio::effect::CommandId;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::kEventFlagNotEmpty;
using aidl::android::hardware::audio::effect::Parameter;
using aidl::android::hardware::audio::effect::Range;
using aidl::android::hardware::audio::effect::State;
using aidl::android::hardware::common::fmq::SynchronizedReadWrite;
using aidl::android::media::audio::common::AudioChannelLayout;
using aidl::android::media::audio::common::AudioFormatDescription;
using aidl::android::media::audio::common::AudioFormatType;
using aidl::android::media::audio::common::AudioUuid;
using aidl::android::media::audio::common::PcmType;
using ::android::audio::utils::toString;
using ::android::hardware::EventFlag;

const AudioFormatDescription kDefaultFormatDescription = {
        .type = AudioFormatType::PCM, .pcm = PcmType::FLOAT_32_BIT, .encoding = ""};

typedef ::android::AidlMessageQueue<IEffect::Status,
                                    ::aidl::android::hardware::common::fmq::SynchronizedReadWrite>
        StatusMQ;
typedef ::android::AidlMessageQueue<float,
                                    ::aidl::android::hardware::common::fmq::SynchronizedReadWrite>
        DataMQ;

static inline std::string getPrefix(Descriptor& descriptor) {
    std::string prefix = "Implementor_" + descriptor.common.implementor + "_name_" +
                         descriptor.common.name + "_UUID_" + toString(descriptor.common.id.uuid);
    return prefix;
}

class EffectHelper {
  public:
    static void create(std::shared_ptr<IFactory> factory, std::shared_ptr<IEffect>& effect,
                       Descriptor& desc, binder_status_t status = EX_NONE) {
        ASSERT_NE(factory, nullptr);
        auto& id = desc.common.id;
        ASSERT_STATUS(status, factory->createEffect(id.uuid, &effect));
        if (status == EX_NONE) {
            ASSERT_NE(effect, nullptr) << toString(id.uuid);
        }
    }

    static void destroyIgnoreRet(std::shared_ptr<IFactory> factory,
                                 std::shared_ptr<IEffect> effect) {
        if (factory && effect) {
            factory->destroyEffect(effect);
        }
    }

    static void destroy(std::shared_ptr<IFactory> factory, std::shared_ptr<IEffect> effect,
                        binder_status_t status = EX_NONE) {
        ASSERT_NE(factory, nullptr);
        ASSERT_NE(effect, nullptr);
        ASSERT_STATUS(status, factory->destroyEffect(effect));
    }

    static void open(std::shared_ptr<IEffect> effect, const Parameter::Common& common,
                     const std::optional<Parameter::Specific>& specific,
                     IEffect::OpenEffectReturn* ret, binder_status_t status = EX_NONE) {
        ASSERT_NE(effect, nullptr);
        ASSERT_STATUS(status, effect->open(common, specific, ret));
    }

    static void open(std::shared_ptr<IEffect> effect, int session = 0,
                     binder_status_t status = EX_NONE) {
        ASSERT_NE(effect, nullptr);
        Parameter::Common common = EffectHelper::createParamCommon(session);
        IEffect::OpenEffectReturn ret;
        ASSERT_NO_FATAL_FAILURE(open(effect, common, std::nullopt /* specific */, &ret, status));
    }

    static void closeIgnoreRet(std::shared_ptr<IEffect> effect) {
        if (effect) {
            effect->close();
        }
    }
    static void close(std::shared_ptr<IEffect> effect, binder_status_t status = EX_NONE) {
        if (effect) {
            ASSERT_STATUS(status, effect->close());
        }
    }
    static void getDescriptor(std::shared_ptr<IEffect> effect, Descriptor& desc,
                              binder_status_t status = EX_NONE) {
        ASSERT_NE(effect, nullptr);
        ASSERT_STATUS(status, effect->getDescriptor(&desc));
    }
    static void expectState(std::shared_ptr<IEffect> effect, State expectState,
                            binder_status_t status = EX_NONE) {
        ASSERT_NE(effect, nullptr);
        State state;
        ASSERT_STATUS(status, effect->getState(&state));
        ASSERT_EQ(expectState, state);
    }
    static void commandIgnoreRet(std::shared_ptr<IEffect> effect, CommandId command) {
        if (effect) {
            effect->command(command);
        }
    }
    static void command(std::shared_ptr<IEffect> effect, CommandId command,
                        binder_status_t status = EX_NONE) {
        ASSERT_NE(effect, nullptr);
        ASSERT_STATUS(status, effect->command(command));
    }
    static void allocateInputData(const Parameter::Common common, std::unique_ptr<DataMQ>& mq,
                                  std::vector<float>& buffer) {
        ASSERT_NE(mq, nullptr);
        auto frameSize = ::aidl::android::hardware::audio::common::getFrameSizeInBytes(
                common.input.base.format, common.input.base.channelMask);
        const size_t floatsToWrite = mq->availableToWrite();
        ASSERT_NE(0UL, floatsToWrite);
        ASSERT_EQ(frameSize * common.input.frameCount, floatsToWrite * sizeof(float));
        buffer.resize(floatsToWrite);
        std::fill(buffer.begin(), buffer.end(), 0x5a);
    }
    static void writeToFmq(std::unique_ptr<StatusMQ>& statusMq, std::unique_ptr<DataMQ>& dataMq,
                           const std::vector<float>& buffer) {
        const size_t available = dataMq->availableToWrite();
        ASSERT_NE(0Ul, available);
        auto bufferFloats = buffer.size();
        auto floatsToWrite = std::min(available, bufferFloats);
        ASSERT_TRUE(dataMq->write(buffer.data(), floatsToWrite));

        EventFlag* efGroup;
        ASSERT_EQ(::android::OK,
                  EventFlag::createEventFlag(statusMq->getEventFlagWord(), &efGroup));
        ASSERT_NE(nullptr, efGroup);
        efGroup->wake(kEventFlagNotEmpty);
        ASSERT_EQ(::android::OK, EventFlag::deleteEventFlag(&efGroup));
    }
    static void readFromFmq(std::unique_ptr<StatusMQ>& statusMq, size_t statusNum,
                            std::unique_ptr<DataMQ>& dataMq, size_t expectFloats,
                            std::vector<float>& buffer,
                            std::optional<int> expectStatus = STATUS_OK) {
        if (0 == statusNum) {
            ASSERT_EQ(0ul, statusMq->availableToRead());
            return;
        }
        IEffect::Status status{};
        ASSERT_TRUE(statusMq->readBlocking(&status, statusNum));
        if (expectStatus.has_value()) {
            ASSERT_EQ(expectStatus.value(), status.status);
        }

        ASSERT_EQ(expectFloats, (unsigned)status.fmqProduced);
        ASSERT_EQ(expectFloats, dataMq->availableToRead());
        if (expectFloats != 0) {
            ASSERT_TRUE(dataMq->read(buffer.data(), expectFloats));
        }
    }
    static Parameter::Common createParamCommon(
            int session = 0, int ioHandle = -1, int iSampleRate = 48000, int oSampleRate = 48000,
            long iFrameCount = 0x100, long oFrameCount = 0x100,
            AudioChannelLayout inputChannelLayout =
                    AudioChannelLayout::make<AudioChannelLayout::layoutMask>(
                            AudioChannelLayout::LAYOUT_STEREO),
            AudioChannelLayout outputChannelLayout =
                    AudioChannelLayout::make<AudioChannelLayout::layoutMask>(
                            AudioChannelLayout::LAYOUT_STEREO)) {
        Parameter::Common common;
        common.session = session;
        common.ioHandle = ioHandle;

        auto& input = common.input;
        auto& output = common.output;
        input.base.sampleRate = iSampleRate;
        input.base.channelMask = inputChannelLayout;
        input.base.format = kDefaultFormatDescription;
        input.frameCount = iFrameCount;
        output.base.sampleRate = oSampleRate;
        output.base.channelMask = outputChannelLayout;
        output.base.format = kDefaultFormatDescription;
        output.frameCount = oFrameCount;
        return common;
    }

    typedef ::android::AidlMessageQueue<
            IEffect::Status, ::aidl::android::hardware::common::fmq::SynchronizedReadWrite>
            StatusMQ;
    typedef ::android::AidlMessageQueue<
            float, ::aidl::android::hardware::common::fmq::SynchronizedReadWrite>
            DataMQ;

    class EffectParam {
      public:
        std::unique_ptr<StatusMQ> statusMQ;
        std::unique_ptr<DataMQ> inputMQ;
        std::unique_ptr<DataMQ> outputMQ;
    };

    template <typename T, Range::Tag tag>
    static bool isParameterValid(const T& target, const Descriptor& desc) {
        if (desc.capability.range.getTag() != tag) {
            return true;
        }
        const auto& ranges = desc.capability.range.get<tag>();
        return inRange(target, ranges);
    }

    /**
     * Add to test value set: (min+max)/2, minimum/maximum numeric limits, and min-1/max+1 if
     * result still in numeric limits after -1/+1.
     * Only use this when the type of test value is basic type (std::is_arithmetic return true).
     */
    template <typename S, typename = std::enable_if_t<std::is_arithmetic_v<S>>>
    static std::set<S> expandTestValueBasic(std::set<S>& s) {
        const auto minLimit = std::numeric_limits<S>::min(),
                   maxLimit = std::numeric_limits<S>::max();
        if (s.size()) {
            const auto min = *s.begin(), max = *s.rbegin();
            s.insert((min & max) + ((min ^ max) >> 1));
            if (min > minLimit + 1) {
                s.insert(min - 1);
            }
            if (max < maxLimit - 1) {
                s.insert(max + 1);
            }
        }
        s.insert(minLimit);
        s.insert(maxLimit);
        return s;
    }

    template <typename T, typename S, Range::Tag R, typename T::Tag tag, typename Functor>
    static std::set<S> getTestValueSet(
            std::vector<std::pair<std::shared_ptr<IFactory>, Descriptor>> kFactoryDescList,
            Functor functor) {
        std::set<S> result;
        for (const auto& [_, desc] : kFactoryDescList) {
            if (desc.capability.range.getTag() == R) {
                const auto& ranges = desc.capability.range.get<R>();
                for (const auto& range : ranges) {
                    if (range.min.getTag() == tag) {
                        result.insert(range.min.template get<tag>());
                    }
                    if (range.max.getTag() == tag) {
                        result.insert(range.max.template get<tag>());
                    }
                }
            }
        }
        return functor(result);
    }

    static void processAndWriteToOutput(std::vector<float>& inputBuffer,
                                        std::vector<float>& outputBuffer,
                                        const std::shared_ptr<IEffect>& mEffect,
                                        IEffect::OpenEffectReturn* mOpenEffectReturn) {
        // Initialize AidlMessagequeues
        auto statusMQ = std::make_unique<EffectHelper::StatusMQ>(mOpenEffectReturn->statusMQ);
        ASSERT_TRUE(statusMQ->isValid());
        auto inputMQ = std::make_unique<EffectHelper::DataMQ>(mOpenEffectReturn->inputDataMQ);
        ASSERT_TRUE(inputMQ->isValid());
        auto outputMQ = std::make_unique<EffectHelper::DataMQ>(mOpenEffectReturn->outputDataMQ);
        ASSERT_TRUE(outputMQ->isValid());

        // Enabling the process
        ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::START));
        ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::PROCESSING));

        // Write from buffer to message queues and calling process
        EXPECT_NO_FATAL_FAILURE(EffectHelper::writeToFmq(statusMQ, inputMQ, inputBuffer));

        // Read the updated message queues into buffer
        EXPECT_NO_FATAL_FAILURE(EffectHelper::readFromFmq(statusMQ, 1, outputMQ,
                                                          outputBuffer.size(), outputBuffer));

        // Disable the process
        ASSERT_NO_FATAL_FAILURE(command(mEffect, CommandId::RESET));
        ASSERT_NO_FATAL_FAILURE(expectState(mEffect, State::IDLE));
    }
};
