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
#include <vector>

#include <Utils.h>
#include <android-base/logging.h>
#include <fmq/AidlMessageQueue.h>

#include <aidl/android/hardware/audio/effect/BnEffect.h>
#include "EffectTypes.h"

namespace aidl::android::hardware::audio::effect {

class EffectContext {
  public:
    typedef ::android::AidlMessageQueue<
            IEffect::Status, ::aidl::android::hardware::common::fmq::SynchronizedReadWrite>
            StatusMQ;
    typedef ::android::AidlMessageQueue<
            float, ::aidl::android::hardware::common::fmq::SynchronizedReadWrite>
            DataMQ;

    EffectContext(size_t statusDepth, const Parameter::Common& common) {
        auto& input = common.input;
        auto& output = common.output;

        LOG_ALWAYS_FATAL_IF(
                input.base.format.pcm != aidl::android::media::audio::common::PcmType::FLOAT_32_BIT,
                "inputFormatNotFloat");
        LOG_ALWAYS_FATAL_IF(output.base.format.pcm !=
                                    aidl::android::media::audio::common::PcmType::FLOAT_32_BIT,
                            "outputFormatNotFloat");
        mInputFrameSize = ::aidl::android::hardware::audio::common::getFrameSizeInBytes(
                input.base.format, input.base.channelMask);
        mOutputFrameSize = ::aidl::android::hardware::audio::common::getFrameSizeInBytes(
                output.base.format, output.base.channelMask);
        // in/outBuffer size in float (FMQ data format defined for DataMQ)
        size_t inBufferSizeInFloat = input.frameCount * mInputFrameSize / sizeof(float);
        size_t outBufferSizeInFloat = output.frameCount * mOutputFrameSize / sizeof(float);

        // only status FMQ use the EventFlag
        mStatusMQ = std::make_shared<StatusMQ>(statusDepth, true /*configureEventFlagWord*/);
        mInputMQ = std::make_shared<DataMQ>(inBufferSizeInFloat);
        mOutputMQ = std::make_shared<DataMQ>(outBufferSizeInFloat);

        if (!mStatusMQ->isValid() || !mInputMQ->isValid() || !mOutputMQ->isValid()) {
            LOG(ERROR) << __func__ << " created invalid FMQ";
        }
        mWorkBuffer.reserve(std::max(inBufferSizeInFloat, outBufferSizeInFloat));
        mCommon = common;
    }
    virtual ~EffectContext() {}

    std::shared_ptr<StatusMQ> getStatusFmq() { return mStatusMQ; }
    std::shared_ptr<DataMQ> getInputDataFmq() { return mInputMQ; }
    std::shared_ptr<DataMQ> getOutputDataFmq() { return mOutputMQ; }

    float* getWorkBuffer() { return static_cast<float*>(mWorkBuffer.data()); }

    // reset buffer status by abandon input data in FMQ
    void resetBuffer() {
        auto buffer = static_cast<float*>(mWorkBuffer.data());
        std::vector<IEffect::Status> status(mStatusMQ->availableToRead());
        mInputMQ->read(buffer, mInputMQ->availableToRead());
    }

    void dupeFmq(IEffect::OpenEffectReturn* effectRet) {
        if (effectRet) {
            effectRet->statusMQ = mStatusMQ->dupeDesc();
            effectRet->inputDataMQ = mInputMQ->dupeDesc();
            effectRet->outputDataMQ = mOutputMQ->dupeDesc();
        }
    }
    size_t getInputFrameSize() { return mInputFrameSize; }
    size_t getOutputFrameSize() { return mOutputFrameSize; }
    int getSessionId() { return mCommon.session; }
    int getIoHandle() { return mCommon.ioHandle; }

    virtual RetCode setOutputDevice(
            const std::vector<aidl::android::media::audio::common::AudioDeviceDescription>&
                    device) {
        mOutputDevice = device;
        return RetCode::SUCCESS;
    }

    virtual std::vector<aidl::android::media::audio::common::AudioDeviceDescription>
    getOutputDevice() {
        return mOutputDevice;
    }

    virtual RetCode setAudioMode(const aidl::android::media::audio::common::AudioMode& mode) {
        mMode = mode;
        return RetCode::SUCCESS;
    }
    virtual aidl::android::media::audio::common::AudioMode getAudioMode() { return mMode; }

    virtual RetCode setAudioSource(const aidl::android::media::audio::common::AudioSource& source) {
        mSource = source;
        return RetCode::SUCCESS;
    }
    virtual aidl::android::media::audio::common::AudioSource getAudioSource() { return mSource; }

    virtual RetCode setVolumeStereo(const Parameter::VolumeStereo& volumeStereo) {
        mVolumeStereo = volumeStereo;
        return RetCode::SUCCESS;
    }
    virtual Parameter::VolumeStereo getVolumeStereo() { return mVolumeStereo; }

    virtual RetCode setCommon(const Parameter::Common& common) {
        mCommon = common;
        LOG(VERBOSE) << __func__ << mCommon.toString();
        return RetCode::SUCCESS;
    }
    virtual Parameter::Common getCommon() {
        LOG(VERBOSE) << __func__ << mCommon.toString();
        return mCommon;
    }

  protected:
    // common parameters
    size_t mInputFrameSize;
    size_t mOutputFrameSize;
    Parameter::Common mCommon;
    std::vector<aidl::android::media::audio::common::AudioDeviceDescription> mOutputDevice;
    aidl::android::media::audio::common::AudioMode mMode;
    aidl::android::media::audio::common::AudioSource mSource;
    Parameter::VolumeStereo mVolumeStereo;

  private:
    // fmq and buffers
    std::shared_ptr<StatusMQ> mStatusMQ;
    std::shared_ptr<DataMQ> mInputMQ;
    std::shared_ptr<DataMQ> mOutputMQ;
    // TODO handle effect process input and output
    // work buffer set by effect instances, the access and update are in same thread
    std::vector<float> mWorkBuffer;
};
}  // namespace aidl::android::hardware::audio::effect
