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

class PortCallbacksHandler : public ::android::bluetooth::audio::aidl::BluetoothAudioPortCallbacks {
  public:
    explicit PortCallbacksHandler(std::shared_ptr<IStreamOutEventCallback> streamCallback)
        : mStreamCallback(streamCallback) {}
    bool hasCallback() const { return mStreamCallback != nullptr; }
    void onRecommendedLatencyModeChanged(
            const std::vector<::aidl::android::hardware::bluetooth::audio::LatencyMode>& modes)
            override;

  private:
    std::shared_ptr<IStreamOutEventCallback> mStreamCallback;
};

class StreamBluetooth : public StreamCommonImpl {
  public:
    static bool checkConfigParams(
            const ::aidl::android::hardware::bluetooth::audio::PcmConfiguration& pcmConfig,
            const ::aidl::android::media::audio::common::AudioConfigBase& config);

    StreamBluetooth(
            StreamContext* context, const Metadata& metadata,
            ModuleBluetooth::BtProfileHandles&& btHandles,
            const std::shared_ptr<::android::bluetooth::audio::aidl::BluetoothAudioPortAidl>&
                    btDeviceProxy,
            const ::aidl::android::hardware::bluetooth::audio::PcmConfiguration& pcmConfig);
    ~StreamBluetooth();

    // Methods of 'DriverInterface'.
    ::android::status_t init(DriverCallbackInterface*) override;
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
    ndk::ScopedAStatus bluetoothParametersUpdated() override;

  protected:
    ndk::ScopedAStatus getRecommendedLatencyModes(
            std::vector<::aidl::android::media::audio::common::AudioLatencyMode>* _aidl_return);
    ndk::ScopedAStatus setLatencyMode(
            ::aidl::android::media::audio::common::AudioLatencyMode in_mode);

    void dump(int fd, const char** args, uint32_t numArgs);

  private:
    const size_t mFrameSizeBytes;
    const bool mIsInput;
    const std::weak_ptr<IBluetoothA2dp> mBluetoothA2dp;
    const std::weak_ptr<IBluetoothLe> mBluetoothLe;
    const size_t mPreferredDataIntervalUs;
    std::shared_ptr<PortCallbacksHandler> mCallbacksHandler;
    std::string mSessionTypeName;
    mutable std::mutex mLock;
    // The lock is also used to serialize calls to the proxy.
    std::shared_ptr<::android::bluetooth::audio::aidl::BluetoothAudioPortAidl> mBtDeviceProxy
            GUARDED_BY(mLock);  // proxy may be null if the stream is not connected to a device
    bool mEnabled GUARDED_BY(mLock) = true;
};

class StreamInBluetooth final : public StreamIn, public StreamBluetooth {
  public:
    friend class ndk::SharedRefBase;

    static int32_t getNominalLatencyMs(size_t dataIntervalUs);

    StreamInBluetooth(
            StreamContext&& context,
            const ::aidl::android::hardware::audio::common::SinkMetadata& sinkMetadata,
            const std::vector<::aidl::android::media::audio::common::MicrophoneInfo>& microphones,
            ModuleBluetooth::BtProfileHandles&& btHandles,
            const std::shared_ptr<::android::bluetooth::audio::aidl::BluetoothAudioPortAidl>&
                    btDeviceProxy,
            const ::aidl::android::hardware::bluetooth::audio::PcmConfiguration& pcmConfig);

  private:
    void onClose(StreamDescriptor::State) override { defaultOnClose(); }
    ndk::ScopedAStatus getActiveMicrophones(
            std::vector<::aidl::android::media::audio::common::MicrophoneDynamicInfo>* _aidl_return)
            override;

    binder_status_t dump(int fd, const char** args, uint32_t numArgs) override;
};

class StreamOutBluetooth final : public StreamOut, public StreamBluetooth {
  public:
    friend class ndk::SharedRefBase;

    static int32_t getNominalLatencyMs(size_t dataIntervalUs);

    StreamOutBluetooth(
            StreamContext&& context,
            const ::aidl::android::hardware::audio::common::SourceMetadata& sourceMetadata,
            const std::optional<::aidl::android::media::audio::common::AudioOffloadInfo>&
                    offloadInfo,
            ModuleBluetooth::BtProfileHandles&& btHandles,
            const std::shared_ptr<::android::bluetooth::audio::aidl::BluetoothAudioPortAidl>&
                    btDeviceProxy,
            const ::aidl::android::hardware::bluetooth::audio::PcmConfiguration& pcmConfig);

  private:
    void onClose(StreamDescriptor::State) override { defaultOnClose(); }

    ndk::ScopedAStatus getRecommendedLatencyModes(
            std::vector<::aidl::android::media::audio::common::AudioLatencyMode>* _aidl_return)
            override;
    ndk::ScopedAStatus setLatencyMode(
            ::aidl::android::media::audio::common::AudioLatencyMode in_mode) override;

    binder_status_t dump(int fd, const char** args, uint32_t numArgs) override;
};

}  // namespace aidl::android::hardware::audio::core
