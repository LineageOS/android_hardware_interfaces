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
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
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

#include "EffectFactoryHelper.h"
#include "TestUtils.h"
#include "pffft.hpp"

using namespace android;
using aidl::android::hardware::audio::effect::CommandId;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::getEffectTypeUuidSpatializer;
using aidl::android::hardware::audio::effect::getRange;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::isRangeValid;
using aidl::android::hardware::audio::effect::kEffectTypeUuidSpatializer;
using aidl::android::hardware::audio::effect::kEventFlagDataMqNotEmpty;
using aidl::android::hardware::audio::effect::kEventFlagDataMqUpdate;
using aidl::android::hardware::audio::effect::kEventFlagNotEmpty;
using aidl::android::hardware::audio::effect::kReopenSupportedVersion;
using aidl::android::hardware::audio::effect::Parameter;
using aidl::android::hardware::audio::effect::Range;
using aidl::android::hardware::audio::effect::Spatializer;
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
    std::replace_if(
            prefix.begin(), prefix.end(), [](const char c) { return !std::isalnum(c); }, '_');
    return prefix;
}

static constexpr float kMaxAudioSampleValue = 1;
static constexpr int kSamplingFrequency = 44100;

class EffectHelper {
  public:
    void create(std::shared_ptr<IFactory> factory, std::shared_ptr<IEffect>& effect,
                Descriptor& desc, binder_status_t status = EX_NONE) {
        ASSERT_NE(factory, nullptr);
        auto& id = desc.common.id;
        ASSERT_STATUS(status, factory->createEffect(id.uuid, &effect));
        if (status == EX_NONE) {
            ASSERT_NE(effect, nullptr) << toString(id.uuid);
            ASSERT_NO_FATAL_FAILURE(expectState(effect, State::INIT));
        }
        mIsSpatializer = id.type == getEffectTypeUuidSpatializer();
        mDescriptor = desc;
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

    void open(std::shared_ptr<IEffect> effect, const Parameter::Common& common,
              const std::optional<Parameter::Specific>& specific, IEffect::OpenEffectReturn* ret,
              binder_status_t status = EX_NONE) {
        ASSERT_NE(effect, nullptr);
        ASSERT_STATUS(status, effect->open(common, specific, ret));
        if (status != EX_NONE) {
            return;
        }

        ASSERT_TRUE(expectState(effect, State::IDLE));
        updateFrameSize(common);
    }

    void open(std::shared_ptr<IEffect> effect, int session = 0, binder_status_t status = EX_NONE) {
        ASSERT_NE(effect, nullptr);
        Parameter::Common common = createParamCommon(session);
        IEffect::OpenEffectReturn ret;
        ASSERT_NO_FATAL_FAILURE(open(effect, common, std::nullopt /* specific */, &ret, status));
    }

    void reopen(std::shared_ptr<IEffect> effect, const Parameter::Common& common,
                IEffect::OpenEffectReturn* ret, binder_status_t status = EX_NONE) {
        ASSERT_NE(effect, nullptr);
        ASSERT_STATUS(status, effect->reopen(ret));
        if (status != EX_NONE) {
            return;
        }
        updateFrameSize(common);
    }

    static void closeIgnoreRet(std::shared_ptr<IEffect> effect) {
        if (effect) {
            effect->close();
        }
    }

    static void close(std::shared_ptr<IEffect> effect, binder_status_t status = EX_NONE) {
        if (effect) {
            ASSERT_STATUS(status, effect->close());
            if (status == EX_NONE) {
                ASSERT_TRUE(expectState(effect, State::INIT));
            }
        }
    }

    static void getDescriptor(std::shared_ptr<IEffect> effect, Descriptor& desc,
                              binder_status_t status = EX_NONE) {
        ASSERT_NE(effect, nullptr);
        ASSERT_STATUS(status, effect->getDescriptor(&desc));
    }

    static bool expectState(std::shared_ptr<IEffect> effect, State expectState) {
        if (effect == nullptr) return false;

        if (State state; EX_NONE != effect->getState(&state).getStatus() || expectState != state) {
            return false;
        }

        return true;
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
        if (status != EX_NONE) {
            return;
        }

        switch (command) {
            case CommandId::START:
                ASSERT_TRUE(expectState(effect, State::PROCESSING));
                break;
            case CommandId::STOP:
                ASSERT_TRUE(expectState(effect, State::IDLE) ||
                            expectState(effect, State::DRAINING));
                break;
            case CommandId::RESET:
                ASSERT_TRUE(expectState(effect, State::IDLE));
                break;
            default:
                return;
        }
    }

    static void writeToFmq(std::unique_ptr<StatusMQ>& statusMq, std::unique_ptr<DataMQ>& dataMq,
                           const std::vector<float>& buffer, int version) {
        const size_t available = dataMq->availableToWrite();
        ASSERT_NE(0Ul, available);
        auto bufferFloats = buffer.size();
        auto floatsToWrite = std::min(available, bufferFloats);
        ASSERT_TRUE(dataMq->write(buffer.data(), floatsToWrite));

        EventFlag* efGroup;
        ASSERT_EQ(::android::OK,
                  EventFlag::createEventFlag(statusMq->getEventFlagWord(), &efGroup));
        ASSERT_NE(nullptr, efGroup);
        efGroup->wake(version >= kReopenSupportedVersion ? kEventFlagDataMqNotEmpty
                                                         : kEventFlagNotEmpty);
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

    static void expectDataMqUpdateEventFlag(std::unique_ptr<StatusMQ>& statusMq) {
        EventFlag* efGroup;
        ASSERT_EQ(::android::OK,
                  EventFlag::createEventFlag(statusMq->getEventFlagWord(), &efGroup));
        ASSERT_NE(nullptr, efGroup);
        uint32_t efState = 0;
        EXPECT_EQ(::android::OK, efGroup->wait(kEventFlagDataMqUpdate, &efState, 1'000'000 /*1ms*/,
                                               true /* retry */));
        EXPECT_TRUE(efState & kEventFlagDataMqUpdate);
    }

    Parameter::Common createParamCommon(int session = 0, int ioHandle = -1, int iSampleRate = 48000,
                                        int oSampleRate = 48000, long iFrameCount = 0x100,
                                        long oFrameCount = 0x100) {
        AudioChannelLayout inputLayout = AudioChannelLayout::make<AudioChannelLayout::layoutMask>(
                AudioChannelLayout::LAYOUT_STEREO);
        AudioChannelLayout outputLayout = inputLayout;

        // query supported input layout and use it as the default parameter in common
        if (mIsSpatializer && isRangeValid<Range::spatializer>(Spatializer::supportedChannelLayout,
                                                               mDescriptor.capability)) {
            const auto layoutRange = getRange<Range::spatializer, Range::SpatializerRange>(
                    mDescriptor.capability, Spatializer::supportedChannelLayout);
            if (std::vector<AudioChannelLayout> layouts;
                layoutRange &&
                0 != (layouts = layoutRange->min.get<Spatializer::supportedChannelLayout>())
                                .size()) {
                inputLayout = layouts[0];
            }
        }

        return createParamCommon(session, ioHandle, iSampleRate, oSampleRate, iFrameCount,
                                 oFrameCount, inputLayout, outputLayout);
    }

    static Parameter::Common createParamCommon(int session, int ioHandle, int iSampleRate,
                                               int oSampleRate, long iFrameCount, long oFrameCount,
                                               AudioChannelLayout inputChannelLayout,
                                               AudioChannelLayout outputChannelLayout) {
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

    template <typename T, typename S, Range::Tag R, typename T::Tag tag>
    static std::set<S> getTestValueSet(
            std::vector<std::pair<std::shared_ptr<IFactory>, Descriptor>> descList) {
        std::set<S> result;
        for (const auto& [_, desc] : descList) {
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
        return result;
    }

    template <typename T, typename S, Range::Tag R, typename T::Tag tag, typename Functor>
    static std::set<S> getTestValueSet(
            std::vector<std::pair<std::shared_ptr<IFactory>, Descriptor>> descList,
            Functor functor) {
        auto result = getTestValueSet<T, S, R, tag>(descList);
        return functor(result);
    }

    // keep writing data to the FMQ until effect transit from DRAINING to IDLE
    static void waitForDrain(std::vector<float>& inputBuffer, std::vector<float>& outputBuffer,
                             const std::shared_ptr<IEffect>& effect,
                             std::unique_ptr<EffectHelper::StatusMQ>& statusMQ,
                             std::unique_ptr<EffectHelper::DataMQ>& inputMQ,
                             std::unique_ptr<EffectHelper::DataMQ>& outputMQ, int version) {
        State state;
        while (effect->getState(&state).getStatus() == EX_NONE && state == State::DRAINING) {
            EXPECT_NO_FATAL_FAILURE(
                    EffectHelper::writeToFmq(statusMQ, inputMQ, inputBuffer, version));
            EXPECT_NO_FATAL_FAILURE(EffectHelper::readFromFmq(
                    statusMQ, 1, outputMQ, outputBuffer.size(), outputBuffer, std::nullopt));
        }
        ASSERT_TRUE(State::IDLE == state);
        EXPECT_NO_FATAL_FAILURE(EffectHelper::readFromFmq(statusMQ, 0, outputMQ, 0, outputBuffer));
        return;
    }

    static void processAndWriteToOutput(std::vector<float>& inputBuffer,
                                        std::vector<float>& outputBuffer,
                                        const std::shared_ptr<IEffect>& effect,
                                        IEffect::OpenEffectReturn* openEffectReturn,
                                        int version = -1, int times = 1,
                                        bool callStopReset = true) {
        // Initialize AidlMessagequeues
        auto statusMQ = std::make_unique<EffectHelper::StatusMQ>(openEffectReturn->statusMQ);
        ASSERT_TRUE(statusMQ->isValid());
        auto inputMQ = std::make_unique<EffectHelper::DataMQ>(openEffectReturn->inputDataMQ);
        ASSERT_TRUE(inputMQ->isValid());
        auto outputMQ = std::make_unique<EffectHelper::DataMQ>(openEffectReturn->outputDataMQ);
        ASSERT_TRUE(outputMQ->isValid());

        // Enabling the process
        ASSERT_NO_FATAL_FAILURE(command(effect, CommandId::START));

        // Write from buffer to message queues and calling process
        if (version == -1) {
            ASSERT_IS_OK(effect->getInterfaceVersion(&version));
        }

        for (int i = 0; i < times; i++) {
            EXPECT_NO_FATAL_FAILURE(
                    EffectHelper::writeToFmq(statusMQ, inputMQ, inputBuffer, version));
            // Read the updated message queues into buffer
            EXPECT_NO_FATAL_FAILURE(EffectHelper::readFromFmq(statusMQ, 1, outputMQ,
                                                              outputBuffer.size(), outputBuffer));
        }

        // Disable the process
        if (callStopReset) {
            ASSERT_NO_FATAL_FAILURE(command(effect, CommandId::STOP));
            EXPECT_NO_FATAL_FAILURE(waitForDrain(inputBuffer, outputBuffer, effect, statusMQ,
                                                 inputMQ, outputMQ, version));
        }

        if (callStopReset) {
            ASSERT_NO_FATAL_FAILURE(command(effect, CommandId::RESET));
        }
    }

    // Find FFT bin indices for testFrequencies and get bin center frequencies
    void roundToFreqCenteredToFftBin(std::vector<int>& testFrequencies,
                                     std::vector<int>& binOffsets, const float kBinWidth) {
        for (size_t i = 0; i < testFrequencies.size(); i++) {
            binOffsets[i] = std::round(testFrequencies[i] / kBinWidth);
            testFrequencies[i] = std::round(binOffsets[i] * kBinWidth);
        }
    }

    // Fill inputBuffer with random values between -maxAudioSampleValue to maxAudioSampleValue
    void generateInputBuffer(std::vector<float>& inputBuffer, size_t startPosition, bool isStrip,
                             size_t channelCount,
                             float maxAudioSampleValue = kMaxAudioSampleValue) {
        size_t increment = isStrip ? 1 /*Fill input at all the channels*/
                                   : channelCount /*Fill input at only one channel*/;

        for (size_t i = startPosition; i < inputBuffer.size(); i += increment) {
            inputBuffer[i] =
                    ((static_cast<float>(std::rand()) / RAND_MAX) * 2 - 1) * maxAudioSampleValue;
        }
    }

    // Generate multitone input between -amplitude to +amplitude using testFrequencies
    // All test frequencies are considered having the same amplitude
    void generateSineWave(const std::vector<int>& testFrequencies, std::vector<float>& input,
                          const float amplitude = 1.0,
                          const int samplingFrequency = kSamplingFrequency) {
        for (size_t i = 0; i < input.size(); i++) {
            input[i] = 0;

            for (size_t j = 0; j < testFrequencies.size(); j++) {
                input[i] += sin(2 * M_PI * testFrequencies[j] * i / samplingFrequency);
            }
            input[i] *= amplitude / testFrequencies.size();
        }
    }

    // Generate single tone input between -amplitude to +amplitude using testFrequency
    void generateSineWave(const int testFrequency, std::vector<float>& input,
                          const float amplitude = 1.0,
                          const int samplingFrequency = kSamplingFrequency) {
        generateSineWave(std::vector<int>{testFrequency}, input, amplitude, samplingFrequency);
    }

    // Use FFT transform to convert the buffer to frequency domain
    // Compute its magnitude at binOffsets
    std::vector<float> calculateMagnitude(const std::vector<float>& buffer,
                                          const std::vector<int>& binOffsets, const int nPointFFT) {
        std::vector<float> fftInput(nPointFFT);
        PFFFT_Setup* inputHandle = pffft_new_setup(nPointFFT, PFFFT_REAL);
        pffft_transform_ordered(inputHandle, buffer.data(), fftInput.data(), nullptr,
                                PFFFT_FORWARD);
        pffft_destroy_setup(inputHandle);
        std::vector<float> bufferMag(binOffsets.size());
        for (size_t i = 0; i < binOffsets.size(); i++) {
            size_t k = binOffsets[i];
            bufferMag[i] = sqrt((fftInput[k * 2] * fftInput[k * 2]) +
                                (fftInput[k * 2 + 1] * fftInput[k * 2 + 1]));
        }

        return bufferMag;
    }

    void updateFrameSize(const Parameter::Common& common) {
        mInputFrameSize = ::aidl::android::hardware::audio::common::getFrameSizeInBytes(
                common.input.base.format, common.input.base.channelMask);
        mInputSamples = common.input.frameCount * mInputFrameSize / sizeof(float);
        mOutputFrameSize = ::aidl::android::hardware::audio::common::getFrameSizeInBytes(
                common.output.base.format, common.output.base.channelMask);
        mOutputSamples = common.output.frameCount * mOutputFrameSize / sizeof(float);
    }

    void generateInput(std::vector<float>& input, float inputFrequency, float samplingFrequency,
                       size_t inputSize = 0) {
        if (inputSize == 0 || inputSize > input.size()) {
            inputSize = input.size();
        }

        for (size_t i = 0; i < inputSize; i++) {
            input[i] = sin(2 * M_PI * inputFrequency * i / samplingFrequency);
        }
    }

    bool mIsSpatializer;
    Descriptor mDescriptor;
    size_t mInputFrameSize, mOutputFrameSize;
    size_t mInputSamples, mOutputSamples;
};
