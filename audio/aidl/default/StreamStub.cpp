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

#define LOG_TAG "AHAL_Stream"
#include <android-base/logging.h>

#include "core-impl/Module.h"
#include "core-impl/StreamStub.h"

using aidl::android::hardware::audio::common::SinkMetadata;
using aidl::android::hardware::audio::common::SourceMetadata;
using aidl::android::media::audio::common::AudioDevice;
using aidl::android::media::audio::common::AudioOffloadInfo;
using aidl::android::media::audio::common::MicrophoneInfo;

namespace aidl::android::hardware::audio::core {

DriverStub::DriverStub(const StreamContext& context, bool isInput)
    : mFrameSizeBytes(context.getFrameSize()), mIsInput(isInput) {}

::android::status_t DriverStub::init() {
    usleep(1000);
    return ::android::OK;
}

::android::status_t DriverStub::drain(StreamDescriptor::DrainMode) {
    usleep(1000);
    return ::android::OK;
}

::android::status_t DriverStub::flush() {
    usleep(1000);
    return ::android::OK;
}

::android::status_t DriverStub::pause() {
    usleep(1000);
    return ::android::OK;
}

::android::status_t DriverStub::transfer(void* buffer, size_t frameCount, size_t* actualFrameCount,
                                         int32_t* latencyMs) {
    usleep(3000);
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

::android::status_t DriverStub::standby() {
    usleep(1000);
    return ::android::OK;
}

::android::status_t DriverStub::setConnectedDevices(
        const std::vector<AudioDevice>& connectedDevices __unused) {
    return ::android::OK;
}

// static
ndk::ScopedAStatus StreamInStub::createInstance(const SinkMetadata& sinkMetadata,
                                                StreamContext&& context,
                                                const std::vector<MicrophoneInfo>& microphones,
                                                std::shared_ptr<StreamIn>* result) {
    std::shared_ptr<StreamIn> stream =
            ndk::SharedRefBase::make<StreamInStub>(sinkMetadata, std::move(context), microphones);
    if (auto status = initInstance(stream); !status.isOk()) {
        return status;
    }
    *result = std::move(stream);
    return ndk::ScopedAStatus::ok();
}

StreamInStub::StreamInStub(const SinkMetadata& sinkMetadata, StreamContext&& context,
                           const std::vector<MicrophoneInfo>& microphones)
    : StreamIn(
              sinkMetadata, std::move(context),
              [](const StreamContext& ctx) -> DriverInterface* {
                  return new DriverStub(ctx, true /*isInput*/);
              },
              [](const StreamContext& ctx, DriverInterface* driver) -> StreamWorkerInterface* {
                  // The default worker implementation is used.
                  return new StreamInWorker(ctx, driver);
              },
              microphones) {}

// static
ndk::ScopedAStatus StreamOutStub::createInstance(const SourceMetadata& sourceMetadata,
                                                 StreamContext&& context,
                                                 const std::optional<AudioOffloadInfo>& offloadInfo,
                                                 std::shared_ptr<StreamOut>* result) {
    std::shared_ptr<StreamOut> stream = ndk::SharedRefBase::make<StreamOutStub>(
            sourceMetadata, std::move(context), offloadInfo);
    if (auto status = initInstance(stream); !status.isOk()) {
        return status;
    }
    *result = std::move(stream);
    return ndk::ScopedAStatus::ok();
}

StreamOutStub::StreamOutStub(const SourceMetadata& sourceMetadata, StreamContext&& context,
                             const std::optional<AudioOffloadInfo>& offloadInfo)
    : StreamOut(
              sourceMetadata, std::move(context),
              [](const StreamContext& ctx) -> DriverInterface* {
                  return new DriverStub(ctx, false /*isInput*/);
              },
              [](const StreamContext& ctx, DriverInterface* driver) -> StreamWorkerInterface* {
                  // The default worker implementation is used.
                  return new StreamOutWorker(ctx, driver);
              },
              offloadInfo) {}

}  // namespace aidl::android::hardware::audio::core
