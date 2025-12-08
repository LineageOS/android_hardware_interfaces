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

#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include "core-impl/Stream.h"
#include "r_submix/SubmixRoute.h"

namespace aidl::android::hardware::audio::core {

class StreamRemoteSubmix : public StreamCommonImpl {
  public:
    StreamRemoteSubmix(StreamContext* context, const Metadata& metadata);
    ~StreamRemoteSubmix();

    // Methods of 'DriverInterface'.
    ::android::status_t init(DriverCallbackInterface*) override;
    ::android::status_t drain(StreamDescriptor::DrainMode) override;
    ::android::status_t flush() override;
    ::android::status_t pause() override;
    ::android::status_t standby() override;
    ::android::status_t start() override;
    ::android::status_t transfer(void* buffer, size_t frameCount, size_t* actualFrameCount,
                                 int32_t* latencyMs) override;
    ::android::status_t refinePosition(StreamDescriptor::Position* position) override;
    void shutdown() override;

    // Overridden methods of 'StreamCommonImpl', called on a Binder thread.
    ndk::ScopedAStatus prepareToClose() override;
    ndk::ScopedAStatus setConnectedDevices(const ConnectedDevices& devices) override;

  private:
    long getDurationInUsForFrameCount(size_t frameCount) const;
    std::shared_ptr<r_submix::SubmixRoute> prepareCurrentRoute(
            const ::aidl::android::media::audio::common::AudioDeviceAddress& deviceAddress);

    size_t getStreamPipeSizeInFrames(const std::shared_ptr<r_submix::SubmixRoute>& currentRoute);
    ::android::status_t outWrite(const std::shared_ptr<r_submix::SubmixRoute>& currentRoute,
                                 void* buffer, size_t frameCount, size_t* actualFrameCount);
    ::android::status_t inRead(const std::shared_ptr<r_submix::SubmixRoute>& currentRoute,
                               void* buffer, size_t frameCount, size_t* actualFrameCount);

    const bool mIsInput;
    const r_submix::AudioConfig mStreamConfig;
    const int mReadAttemptSleepUs;

    // Limit for the number of error log entries to avoid spamming the logs.
    static constexpr int kMaxErrorLogs = 5;

    mutable std::mutex mLock;
    std::shared_ptr<r_submix::SubmixRoute> mCurrentRoute GUARDED_BY(mLock);

    // Used by the worker thread only.
    int64_t mStartTimeNs = 0;
    long mFramesSinceStart = 0;
    int mReadErrorCount = 0;
    int mReadFailureCount = 0;
    int mWriteShutdownCount = 0;
    bool mSkipNextTransfer = false;
};

class StreamInRemoteSubmix final : public StreamIn, public StreamRemoteSubmix {
  public:
    friend class ndk::SharedRefBase;
    StreamInRemoteSubmix(
            StreamContext&& context,
            const ::aidl::android::hardware::audio::common::SinkMetadata& sinkMetadata,
            const std::vector<::aidl::android::media::audio::common::MicrophoneInfo>& microphones);

  private:
    void onClose(StreamDescriptor::State) override { defaultOnClose(); }
    ndk::ScopedAStatus getActiveMicrophones(
            std::vector<::aidl::android::media::audio::common::MicrophoneDynamicInfo>* _aidl_return)
            override;
};

class StreamOutRemoteSubmix final : public StreamOut, public StreamRemoteSubmix {
  public:
    friend class ndk::SharedRefBase;
    StreamOutRemoteSubmix(
            StreamContext&& context,
            const ::aidl::android::hardware::audio::common::SourceMetadata& sourceMetadata,
            const std::optional<::aidl::android::media::audio::common::AudioOffloadInfo>&
                    offloadInfo);

  private:
    void onClose(StreamDescriptor::State) override { defaultOnClose(); }
};

}  // namespace aidl::android::hardware::audio::core
