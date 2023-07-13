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

#include <atomic>
#include <mutex>
#include <vector>

#include <aidl/android/media/audio/common/AudioChannelLayout.h>

#include "StreamAlsa.h"

namespace aidl::android::hardware::audio::core {

class StreamUsb : public StreamAlsa {
  public:
    StreamUsb(const Metadata& metadata, StreamContext&& context);
    // Methods of 'DriverInterface'.
    ::android::status_t drain(StreamDescriptor::DrainMode) override;
    ::android::status_t flush() override;
    ::android::status_t pause() override;
    ::android::status_t transfer(void* buffer, size_t frameCount, size_t* actualFrameCount,
                                 int32_t* latencyMs) override;

    // Overridden methods of 'StreamCommonImpl', called on a Binder thread.
    ndk::ScopedAStatus setConnectedDevices(const ConnectedDevices& devices) override;

  protected:
    std::vector<alsa::DeviceProfile> getDeviceProfiles() override;

    mutable std::mutex mLock;
    std::vector<alsa::DeviceProfile> mConnectedDeviceProfiles GUARDED_BY(mLock);
    std::atomic<bool> mConnectedDevicesUpdated = false;
};

class StreamInUsb final : public StreamUsb, public StreamIn {
  public:
    friend class ndk::SharedRefBase;
    StreamInUsb(
            const ::aidl::android::hardware::audio::common::SinkMetadata& sinkMetadata,
            StreamContext&& context,
            const std::vector<::aidl::android::media::audio::common::MicrophoneInfo>& microphones);

  private:
    ndk::ScopedAStatus getActiveMicrophones(
            std::vector<::aidl::android::media::audio::common::MicrophoneDynamicInfo>* _aidl_return)
            override;
};

class StreamOutUsb final : public StreamUsb, public StreamOut {
  public:
    friend class ndk::SharedRefBase;
    StreamOutUsb(const ::aidl::android::hardware::audio::common::SourceMetadata& sourceMetadata,
                 StreamContext&& context,
                 const std::optional<::aidl::android::media::audio::common::AudioOffloadInfo>&
                         offloadInfo);

  private:
    ndk::ScopedAStatus getHwVolume(std::vector<float>* _aidl_return) override;
    ndk::ScopedAStatus setHwVolume(const std::vector<float>& in_channelVolumes) override;

    const int mChannelCount;
    std::vector<float> mHwVolumes;
};

}  // namespace aidl::android::hardware::audio::core
