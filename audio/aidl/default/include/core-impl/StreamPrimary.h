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

#include <android-base/thread_annotations.h>

#include "DriverStubImpl.h"
#include "StreamAlsa.h"
#include "primary/PrimaryMixer.h"

namespace aidl::android::hardware::audio::core {

class StreamPrimary : public StreamAlsa {
  public:
    StreamPrimary(StreamContext* context, const Metadata& metadata);

    // Methods of 'DriverInterface'.
    ::android::status_t init() override;
    ::android::status_t drain(StreamDescriptor::DrainMode mode) override;
    ::android::status_t flush() override;
    ::android::status_t pause() override;
    ::android::status_t standby() override;
    ::android::status_t start() override;
    ::android::status_t transfer(void* buffer, size_t frameCount, size_t* actualFrameCount,
                                 int32_t* latencyMs) override;
    ::android::status_t refinePosition(StreamDescriptor::Position* position) override;
    void shutdown() override;

    // Overridden methods of 'StreamCommonImpl', called on a Binder thread.
    ndk::ScopedAStatus setConnectedDevices(const ConnectedDevices& devices) override;

  protected:
    std::vector<alsa::DeviceProfile> getDeviceProfiles() override;
    bool isStubStream();

    const bool mIsAsynchronous;
    int64_t mStartTimeNs = 0;
    long mFramesSinceStart = 0;
    bool mSkipNextTransfer = false;

  private:
    using AlsaDeviceId = std::pair<int, int>;

    static constexpr StreamPrimary::AlsaDeviceId kDefaultCardAndDeviceId{
            primary::PrimaryMixer::kAlsaCard, primary::PrimaryMixer::kAlsaDevice};
    static constexpr StreamPrimary::AlsaDeviceId kStubDeviceId{
            primary::PrimaryMixer::kInvalidAlsaCard, primary::PrimaryMixer::kInvalidAlsaDevice};

    static AlsaDeviceId getCardAndDeviceId(
            const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices);
    static bool useStubStream(bool isInput,
                              const ::aidl::android::media::audio::common::AudioDevice& device);

    bool isStubStreamOnWorker() const { return mCurrAlsaDeviceId == kStubDeviceId; }

    DriverStubImpl mStubDriver;
    mutable std::mutex mLock;
    AlsaDeviceId mAlsaDeviceId GUARDED_BY(mLock) = kStubDeviceId;

    // Used by the worker thread only.
    AlsaDeviceId mCurrAlsaDeviceId = kStubDeviceId;
};

class StreamInPrimary final : public StreamIn, public StreamPrimary, public StreamInHwGainHelper {
  public:
    friend class ndk::SharedRefBase;
    StreamInPrimary(
            StreamContext&& context,
            const ::aidl::android::hardware::audio::common::SinkMetadata& sinkMetadata,
            const std::vector<::aidl::android::media::audio::common::MicrophoneInfo>& microphones);

  private:
    void onClose(StreamDescriptor::State) override { defaultOnClose(); }

    ndk::ScopedAStatus getHwGain(std::vector<float>* _aidl_return) override;
    ndk::ScopedAStatus setHwGain(const std::vector<float>& in_channelGains) override;
};

class StreamOutPrimary final : public StreamOut,
                               public StreamPrimary,
                               public StreamOutHwVolumeHelper {
  public:
    friend class ndk::SharedRefBase;
    StreamOutPrimary(StreamContext&& context,
                     const ::aidl::android::hardware::audio::common::SourceMetadata& sourceMetadata,
                     const std::optional<::aidl::android::media::audio::common::AudioOffloadInfo>&
                             offloadInfo);

  private:
    void onClose(StreamDescriptor::State) override { defaultOnClose(); }

    ndk::ScopedAStatus getHwVolume(std::vector<float>* _aidl_return) override;
    ndk::ScopedAStatus setHwVolume(const std::vector<float>& in_channelVolumes) override;
};

}  // namespace aidl::android::hardware::audio::core
