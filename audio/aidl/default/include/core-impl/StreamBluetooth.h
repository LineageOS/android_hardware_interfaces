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

#include <aidl/android/hardware/audio/core/IBluetooth.h>
#include <aidl/android/hardware/audio/core/IBluetoothA2dp.h>
#include <aidl/android/hardware/audio/core/IBluetoothLe.h>

#include "core-impl/DevicePortProxy.h"
#include "core-impl/ModuleBluetooth.h"
#include "core-impl/Stream.h"

namespace aidl::android::hardware::audio::core {

class StreamBluetooth : public StreamCommonImpl {
  public:
    StreamBluetooth(StreamContext* context, const Metadata& metadata,
                    ModuleBluetooth::BtProfileHandles&& btHandles);
    // Methods of 'DriverInterface'.
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
    ndk::ScopedAStatus updateMetadataCommon(const Metadata& metadata) override;
    ndk::ScopedAStatus prepareToClose() override;
    const ConnectedDevices& getConnectedDevices() const override;
    ndk::ScopedAStatus setConnectedDevices(const ConnectedDevices& devices) override;
    ndk::ScopedAStatus bluetoothParametersUpdated() override;

  private:
    // Audio Pcm Config
    const uint32_t mSampleRate;
    const ::aidl::android::media::audio::common::AudioChannelLayout mChannelLayout;
    const ::aidl::android::media::audio::common::AudioFormatDescription mFormat;
    const size_t mFrameSizeBytes;
    const bool mIsInput;
    const std::weak_ptr<IBluetoothA2dp> mBluetoothA2dp;
    const std::weak_ptr<IBluetoothLe> mBluetoothLe;
    size_t mPreferredDataIntervalUs;
    size_t mPreferredFrameCount;

    mutable std::mutex mLock;
    bool mIsInitialized GUARDED_BY(mLock);
    bool mIsReadyToClose GUARDED_BY(mLock);
    std::vector<std::shared_ptr<::android::bluetooth::audio::aidl::BluetoothAudioPortAidl>>
            mBtDeviceProxies GUARDED_BY(mLock);

    ::android::status_t initialize() REQUIRES(mLock);
    bool checkConfigParams(::aidl::android::hardware::bluetooth::audio::PcmConfiguration& config);
};

class StreamInBluetooth final : public StreamIn, public StreamBluetooth {
  public:
    friend class ndk::SharedRefBase;
    StreamInBluetooth(
            StreamContext&& context,
            const ::aidl::android::hardware::audio::common::SinkMetadata& sinkMetadata,
            const std::vector<::aidl::android::media::audio::common::MicrophoneInfo>& microphones,
            ModuleBluetooth::BtProfileHandles&& btHandles);

  private:
    void onClose(StreamDescriptor::State) override { defaultOnClose(); }
    ndk::ScopedAStatus getActiveMicrophones(
            std::vector<::aidl::android::media::audio::common::MicrophoneDynamicInfo>* _aidl_return)
            override;
};

class StreamOutBluetooth final : public StreamOut, public StreamBluetooth {
  public:
    friend class ndk::SharedRefBase;
    StreamOutBluetooth(
            StreamContext&& context,
            const ::aidl::android::hardware::audio::common::SourceMetadata& sourceMetadata,
            const std::optional<::aidl::android::media::audio::common::AudioOffloadInfo>&
                    offloadInfo,
            ModuleBluetooth::BtProfileHandles&& btHandles);

  private:
    void onClose(StreamDescriptor::State) override { defaultOnClose(); }
};

}  // namespace aidl::android::hardware::audio::core
