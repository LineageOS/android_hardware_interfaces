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

#include <limits>

#define LOG_TAG "AHAL_StreamUsb"
#include <android-base/logging.h>
#include <error/expected_utils.h>

#include "UsbAlsaMixerControl.h"
#include "core-impl/StreamUsb.h"

using aidl::android::hardware::audio::common::SinkMetadata;
using aidl::android::hardware::audio::common::SourceMetadata;
using aidl::android::media::audio::common::AudioDevice;
using aidl::android::media::audio::common::AudioOffloadInfo;
using aidl::android::media::audio::common::MicrophoneDynamicInfo;
using aidl::android::media::audio::common::MicrophoneInfo;

namespace aidl::android::hardware::audio::core {

StreamUsb::StreamUsb(StreamContext* context, const Metadata& metadata)
    : StreamAlsa(context, metadata, 1 /*readWriteRetries*/) {}

ndk::ScopedAStatus StreamUsb::setConnectedDevices(
        const std::vector<AudioDevice>& connectedDevices) {
    if (mIsInput && connectedDevices.size() > 1) {
        LOG(ERROR) << __func__ << ": wrong device size(" << connectedDevices.size()
                   << ") for input stream";
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    std::vector<alsa::DeviceProfile> connectedDeviceProfiles;
    for (const auto& connectedDevice : connectedDevices) {
        auto profile = alsa::getDeviceProfile(connectedDevice, mIsInput);
        if (!profile.has_value()) {
            LOG(ERROR) << __func__
                       << ": unsupported device address=" << connectedDevice.address.toString();
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }
        connectedDeviceProfiles.push_back(*profile);
    }
    RETURN_STATUS_IF_ERROR(setConnectedDevices(connectedDevices));
    std::lock_guard guard(mLock);
    mConnectedDeviceProfiles = std::move(connectedDeviceProfiles);
    mConnectedDevicesUpdated.store(true, std::memory_order_release);
    return ndk::ScopedAStatus::ok();
}

::android::status_t StreamUsb::transfer(void* buffer, size_t frameCount, size_t* actualFrameCount,
                                        int32_t* latencyMs) {
    if (mConnectedDevicesUpdated.load(std::memory_order_acquire)) {
        // 'setConnectedDevices' was called. I/O will be restarted.
        *actualFrameCount = 0;
        *latencyMs = StreamDescriptor::LATENCY_UNKNOWN;
        return ::android::OK;
    }
    return StreamAlsa::transfer(buffer, frameCount, actualFrameCount, latencyMs);
}

std::vector<alsa::DeviceProfile> StreamUsb::getDeviceProfiles() {
    std::vector<alsa::DeviceProfile> connectedDevices;
    {
        std::lock_guard guard(mLock);
        connectedDevices = mConnectedDeviceProfiles;
        mConnectedDevicesUpdated.store(false, std::memory_order_release);
    }
    return connectedDevices;
}

StreamInUsb::StreamInUsb(StreamContext&& context, const SinkMetadata& sinkMetadata,
                         const std::vector<MicrophoneInfo>& microphones)
    : StreamIn(std::move(context), microphones), StreamUsb(&mContextInstance, sinkMetadata) {}

ndk::ScopedAStatus StreamInUsb::getActiveMicrophones(
        std::vector<MicrophoneDynamicInfo>* _aidl_return __unused) {
    LOG(DEBUG) << __func__ << ": not supported";
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

StreamOutUsb::StreamOutUsb(StreamContext&& context, const SourceMetadata& sourceMetadata,
                           const std::optional<AudioOffloadInfo>& offloadInfo)
    : StreamOut(std::move(context), offloadInfo),
      StreamUsb(&mContextInstance, sourceMetadata),
      StreamOutHwVolumeHelper(&mContextInstance) {}

ndk::ScopedAStatus StreamOutUsb::getHwVolume(std::vector<float>* _aidl_return) {
    return getHwVolumeImpl(_aidl_return);
}

ndk::ScopedAStatus StreamOutUsb::setHwVolume(const std::vector<float>& in_channelVolumes) {
    auto currentVolumes = mHwVolumes;
    RETURN_STATUS_IF_ERROR(setHwVolumeImpl(in_channelVolumes));
    // Avoid using mConnectedDeviceProfiles because it requires a lock.
    for (const auto& device : getConnectedDevices()) {
        if (auto deviceProfile = alsa::getDeviceProfile(device, mIsInput);
            deviceProfile.has_value()) {
            if (auto result = usb::UsbAlsaMixerControl::getInstance().setVolumes(
                        deviceProfile->card, in_channelVolumes);
                !result.isOk()) {
                LOG(ERROR) << __func__
                           << ": failed to set volume for device address=" << *deviceProfile;
                mHwVolumes = currentVolumes;
                return result;
            }
        }
    }
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::audio::core
