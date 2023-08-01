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

#include <mutex>
#include <vector>

#include "core-impl/Stream.h"
#include "r_submix/SubmixRoute.h"

namespace aidl::android::hardware::audio::core {

using aidl::android::hardware::audio::core::r_submix::AudioConfig;
using aidl::android::hardware::audio::core::r_submix::SubmixRoute;

class StreamRemoteSubmix : public StreamCommonImpl {
  public:
    StreamRemoteSubmix(const Metadata& metadata, StreamContext&& context);

    ::android::status_t init() override;
    ::android::status_t drain(StreamDescriptor::DrainMode) override;
    ::android::status_t flush() override;
    ::android::status_t pause() override;
    ::android::status_t standby() override;
    ::android::status_t start() override;
    ::android::status_t transfer(void* buffer, size_t frameCount, size_t* actualFrameCount,
                                 int32_t* latencyMs) override;
    void shutdown() override;

    // Overridden methods of 'StreamCommonImpl', called on a Binder thread.
    ndk::ScopedAStatus prepareToClose() override;

  private:
    size_t getPipeSizeInFrames();
    size_t getStreamPipeSizeInFrames();
    ::android::status_t outWrite(void* buffer, size_t frameCount, size_t* actualFrameCount);
    ::android::status_t inRead(void* buffer, size_t frameCount, size_t* actualFrameCount);

    const int mPortId;
    const bool mIsInput;
    AudioConfig mStreamConfig;
    std::shared_ptr<SubmixRoute> mCurrentRoute = nullptr;

    // Mutex lock to protect vector of submix routes, each of these submix routes have their mutex
    // locks and none of the mutex locks should be taken together.
    static std::mutex sSubmixRoutesLock;
    static std::map<int32_t, std::shared_ptr<SubmixRoute>> sSubmixRoutes
            GUARDED_BY(sSubmixRoutesLock);

    // limit for number of read error log entries to avoid spamming the logs
    static constexpr int kMaxReadErrorLogs = 5;
    // The duration of kMaxReadFailureAttempts * READ_ATTEMPT_SLEEP_MS must be strictly inferior
    // to the duration of a record buffer at the current record sample rate (of the device, not of
    // the recording itself). Here we have: 3 * 5ms = 15ms < 1024 frames * 1000 / 48000 = 21.333ms
    static constexpr int kMaxReadFailureAttempts = 3;
    // 5ms between two read attempts when pipe is empty
    static constexpr int kReadAttemptSleepUs = 5000;
};

class StreamInRemoteSubmix final : public StreamRemoteSubmix, public StreamIn {
  public:
    friend class ndk::SharedRefBase;
    StreamInRemoteSubmix(
            const ::aidl::android::hardware::audio::common::SinkMetadata& sinkMetadata,
            StreamContext&& context,
            const std::vector<::aidl::android::media::audio::common::MicrophoneInfo>& microphones);

  private:
    ndk::ScopedAStatus getActiveMicrophones(
            std::vector<::aidl::android::media::audio::common::MicrophoneDynamicInfo>* _aidl_return)
            override;
};

class StreamOutRemoteSubmix final : public StreamRemoteSubmix, public StreamOut {
  public:
    friend class ndk::SharedRefBase;
    StreamOutRemoteSubmix(
            const ::aidl::android::hardware::audio::common::SourceMetadata& sourceMetadata,
            StreamContext&& context,
            const std::optional<::aidl::android::media::audio::common::AudioOffloadInfo>&
                    offloadInfo);
};

}  // namespace aidl::android::hardware::audio::core
