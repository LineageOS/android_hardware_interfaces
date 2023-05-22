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

#include <aidl/android/media/audio/common/AudioChannelLayout.h>

#include "core-impl/Stream.h"

extern "C" {
#include <tinyalsa/pcm.h>
#include "alsa_device_proxy.h"
}

namespace aidl::android::hardware::audio::core {

class DriverUsb : public DriverInterface {
  public:
    DriverUsb(const StreamContext& context, bool isInput);
    ::android::status_t init() override;
    ::android::status_t drain(StreamDescriptor::DrainMode) override;
    ::android::status_t flush() override;
    ::android::status_t pause() override;
    ::android::status_t transfer(void* buffer, size_t frameCount, size_t* actualFrameCount,
                                 int32_t* latencyMs) override;
    ::android::status_t standby() override;
    // Note: called on a different thread.
    ::android::status_t setConnectedDevices(
            const std::vector<::aidl::android::media::audio::common::AudioDevice>& connectedDevices)
            override;

  private:
    ::android::status_t exitStandby();

    std::mutex mLock;

    const size_t mFrameSizeBytes;
    std::optional<struct pcm_config> mConfig;
    const bool mIsInput;
    // Cached device addresses for connected devices.
    std::vector<::aidl::android::media::audio::common::AudioDeviceAddress> mConnectedDevices
            GUARDED_BY(mLock);
    std::vector<std::shared_ptr<alsa_device_proxy>> mAlsaDeviceProxies GUARDED_BY(mLock);
    bool mIsStandby = true;
};

class StreamInUsb final : public StreamIn {
    ndk::ScopedAStatus getActiveMicrophones(
            std::vector<::aidl::android::media::audio::common::MicrophoneDynamicInfo>* _aidl_return)
            override;

  public:
    static ndk::ScopedAStatus createInstance(
            const ::aidl::android::hardware::audio::common::SinkMetadata& sinkMetadata,
            StreamContext&& context,
            const std::vector<::aidl::android::media::audio::common::MicrophoneInfo>& microphones,
            std::shared_ptr<StreamIn>* result);

  private:
    friend class ndk::SharedRefBase;
    StreamInUsb(
            const ::aidl::android::hardware::audio::common::SinkMetadata& sinkMetadata,
            StreamContext&& context,
            const std::vector<::aidl::android::media::audio::common::MicrophoneInfo>& microphones);
};

class StreamOutUsb final : public StreamOut {
  public:
    static ndk::ScopedAStatus createInstance(
            const ::aidl::android::hardware::audio::common::SourceMetadata& sourceMetadata,
            StreamContext&& context,
            const std::optional<::aidl::android::media::audio::common::AudioOffloadInfo>&
                    offloadInfo,
            std::shared_ptr<StreamOut>* result);

  private:
    friend class ndk::SharedRefBase;
    StreamOutUsb(const ::aidl::android::hardware::audio::common::SourceMetadata& sourceMetadata,
                 StreamContext&& context,
                 const std::optional<::aidl::android::media::audio::common::AudioOffloadInfo>&
                         offloadInfo);

    ndk::ScopedAStatus getHwVolume(std::vector<float>* _aidl_return) override;
    ndk::ScopedAStatus setHwVolume(const std::vector<float>& in_channelVolumes) override;

    int mChannelCount;
    std::vector<float> mHwVolumes;
};

}  // namespace aidl::android::hardware::audio::core
