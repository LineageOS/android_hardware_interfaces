/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include <cmath>

#define LOG_TAG "AHAL_Stream"
#include <android-base/logging.h>
#include <audio_utils/clock.h>

#include "core-impl/Module.h"
#include "core-impl/StreamStub.h"

using aidl::android::hardware::audio::common::SinkMetadata;
using aidl::android::hardware::audio::common::SourceMetadata;
using aidl::android::media::audio::common::AudioDevice;
using aidl::android::media::audio::common::AudioOffloadInfo;
using aidl::android::media::audio::common::MicrophoneInfo;

namespace aidl::android::hardware::audio::core {

StreamStub::StreamStub(StreamContext* context, const Metadata& metadata)
    : StreamCommonImpl(context, metadata),
      mBufferSizeFrames(getContext().getBufferSizeInFrames()),
      mFrameSizeBytes(getContext().getFrameSize()),
      mSampleRate(getContext().getSampleRate()),
      mIsAsynchronous(!!getContext().getAsyncCallback()),
      mIsInput(isInput(metadata)) {}

::android::status_t StreamStub::init() {
    mIsInitialized = true;
    return ::android::OK;
}

::android::status_t StreamStub::drain(StreamDescriptor::DrainMode) {
    if (!mIsInitialized) {
        LOG(FATAL) << __func__ << ": must not happen for an uninitialized driver";
    }
    if (!mIsInput) {
        if (!mIsAsynchronous) {
            static constexpr float kMicrosPerSecond = MICROS_PER_SECOND;
            const size_t delayUs = static_cast<size_t>(
                    std::roundf(mBufferSizeFrames * kMicrosPerSecond / mSampleRate));
            usleep(delayUs);
        } else {
            usleep(500);
        }
    }
    return ::android::OK;
}

::android::status_t StreamStub::flush() {
    if (!mIsInitialized) {
        LOG(FATAL) << __func__ << ": must not happen for an uninitialized driver";
    }
    return ::android::OK;
}

::android::status_t StreamStub::pause() {
    if (!mIsInitialized) {
        LOG(FATAL) << __func__ << ": must not happen for an uninitialized driver";
    }
    return ::android::OK;
}

::android::status_t StreamStub::standby() {
    if (!mIsInitialized) {
        LOG(FATAL) << __func__ << ": must not happen for an uninitialized driver";
    }
    usleep(500);
    mIsStandby = true;
    return ::android::OK;
}

::android::status_t StreamStub::start() {
    if (!mIsInitialized) {
        LOG(FATAL) << __func__ << ": must not happen for an uninitialized driver";
    }
    usleep(500);
    mIsStandby = false;
    return ::android::OK;
}

::android::status_t StreamStub::transfer(void* buffer, size_t frameCount, size_t* actualFrameCount,
                                         int32_t*) {
    if (!mIsInitialized) {
        LOG(FATAL) << __func__ << ": must not happen for an uninitialized driver";
    }
    if (mIsStandby) {
        LOG(FATAL) << __func__ << ": must not happen while in standby";
    }
    static constexpr float kMicrosPerSecond = MICROS_PER_SECOND;
    static constexpr float kScaleFactor = .8f;
    if (mIsAsynchronous) {
        usleep(500);
    } else {
        const size_t delayUs = static_cast<size_t>(
                std::roundf(kScaleFactor * frameCount * kMicrosPerSecond / mSampleRate));
        usleep(delayUs);
    }
    if (mIsInput) {
        uint8_t* byteBuffer = static_cast<uint8_t*>(buffer);
        for (size_t i = 0; i < frameCount * mFrameSizeBytes; ++i) {
            byteBuffer[i] = std::rand() % 255;
        }
    }
    *actualFrameCount = frameCount;
    return ::android::OK;
}

void StreamStub::shutdown() {
    mIsInitialized = false;
}

StreamInStub::StreamInStub(StreamContext&& context, const SinkMetadata& sinkMetadata,
                           const std::vector<MicrophoneInfo>& microphones)
    : StreamIn(std::move(context), microphones), StreamStub(&mContextInstance, sinkMetadata) {}

StreamOutStub::StreamOutStub(StreamContext&& context, const SourceMetadata& sourceMetadata,
                             const std::optional<AudioOffloadInfo>& offloadInfo)
    : StreamOut(std::move(context), offloadInfo), StreamStub(&mContextInstance, sourceMetadata) {}

}  // namespace aidl::android::hardware::audio::core
