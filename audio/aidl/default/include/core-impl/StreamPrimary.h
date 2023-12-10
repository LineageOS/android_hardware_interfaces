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

#include <vector>

#include "StreamAlsa.h"
#include "StreamSwitcher.h"

namespace aidl::android::hardware::audio::core {

class StreamPrimary : public StreamAlsa {
  public:
    StreamPrimary(StreamContext* context, const Metadata& metadata);

  protected:
    std::vector<alsa::DeviceProfile> getDeviceProfiles() override;

    const bool mIsInput;
};

class StreamInPrimary final : public StreamIn, public StreamSwitcher, public StreamInHwGainHelper {
  public:
    friend class ndk::SharedRefBase;
    StreamInPrimary(
            StreamContext&& context,
            const ::aidl::android::hardware::audio::common::SinkMetadata& sinkMetadata,
            const std::vector<::aidl::android::media::audio::common::MicrophoneInfo>& microphones);

  private:
    static bool useStubStream(const ::aidl::android::media::audio::common::AudioDevice& device);

    DeviceSwitchBehavior switchCurrentStream(
            const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices)
            override;
    std::unique_ptr<StreamCommonInterfaceEx> createNewStream(
            const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices,
            StreamContext* context, const Metadata& metadata) override;
    void onClose(StreamDescriptor::State) override { defaultOnClose(); }

    ndk::ScopedAStatus getHwGain(std::vector<float>* _aidl_return) override;
    ndk::ScopedAStatus setHwGain(const std::vector<float>& in_channelGains) override;
};

class StreamOutPrimary final : public StreamOut,
                               public StreamSwitcher,
                               public StreamOutHwVolumeHelper {
  public:
    friend class ndk::SharedRefBase;
    StreamOutPrimary(StreamContext&& context,
                     const ::aidl::android::hardware::audio::common::SourceMetadata& sourceMetadata,
                     const std::optional<::aidl::android::media::audio::common::AudioOffloadInfo>&
                             offloadInfo);

  private:
    static bool useStubStream(const ::aidl::android::media::audio::common::AudioDevice& device);

    DeviceSwitchBehavior switchCurrentStream(
            const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices)
            override;
    std::unique_ptr<StreamCommonInterfaceEx> createNewStream(
            const std::vector<::aidl::android::media::audio::common::AudioDevice>& devices,
            StreamContext* context, const Metadata& metadata) override;
    void onClose(StreamDescriptor::State) override { defaultOnClose(); }

    ndk::ScopedAStatus getHwVolume(std::vector<float>* _aidl_return) override;
    ndk::ScopedAStatus setHwVolume(const std::vector<float>& in_channelVolumes) override;
};

}  // namespace aidl::android::hardware::audio::core
