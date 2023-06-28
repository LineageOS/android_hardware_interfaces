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

StreamStub::StreamStub(const Metadata& metadata, StreamContext&& context)
    : StreamCommonImpl(metadata, std::move(context)),
      mFrameSizeBytes(context.getFrameSize()),
      mSampleRate(context.getSampleRate()),
      mIsAsynchronous(!!context.getAsyncCallback()),
      mIsInput(isInput(metadata)) {}

::android::status_t StreamStub::init() {
    usleep(500);
    return ::android::OK;
}

::android::status_t StreamStub::drain(StreamDescriptor::DrainMode) {
    usleep(500);
    return ::android::OK;
}

::android::status_t StreamStub::flush() {
    usleep(500);
    return ::android::OK;
}

::android::status_t StreamStub::pause() {
    usleep(500);
    return ::android::OK;
}

::android::status_t StreamStub::transfer(void* buffer, size_t frameCount, size_t* actualFrameCount,
                                         int32_t* latencyMs) {
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
    *latencyMs = Module::kLatencyMs;
    return ::android::OK;
}

::android::status_t StreamStub::standby() {
    usleep(500);
    return ::android::OK;
}

void StreamStub::shutdown() {}

StreamInStub::StreamInStub(const SinkMetadata& sinkMetadata, StreamContext&& context,
                           const std::vector<MicrophoneInfo>& microphones)
    : StreamStub(sinkMetadata, std::move(context)), StreamIn(microphones) {}

StreamOutStub::StreamOutStub(const SourceMetadata& sourceMetadata, StreamContext&& context,
                             const std::optional<AudioOffloadInfo>& offloadInfo)
    : StreamStub(sourceMetadata, std::move(context)), StreamOut(offloadInfo) {}

}  // namespace aidl::android::hardware::audio::core
