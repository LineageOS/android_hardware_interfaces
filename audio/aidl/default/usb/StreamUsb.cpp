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

#include <Utils.h>
#include <error/expected_utils.h>

#include "UsbAlsaMixerControl.h"
#include "core-impl/StreamUsb.h"

using aidl::android::hardware::audio::common::getChannelCount;
using aidl::android::hardware::audio::common::SinkMetadata;
using aidl::android::hardware::audio::common::SourceMetadata;
using aidl::android::media::audio::common::AudioDevice;
using aidl::android::media::audio::common::AudioOffloadInfo;
using aidl::android::media::audio::common::MicrophoneDynamicInfo;
using aidl::android::media::audio::common::MicrophoneInfo;

namespace aidl::android::hardware::audio::core {

StreamUsb::StreamUsb(const Metadata& metadata, StreamContext&& context)
    : StreamAlsa(metadata, std::move(context)) {}

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
    RETURN_STATUS_IF_ERROR(StreamCommonImpl::setConnectedDevices(connectedDevices));
    std::lock_guard guard(mLock);
    mConnectedDeviceProfiles = std::move(connectedDeviceProfiles);
    mConnectedDevicesUpdated.store(true, std::memory_order_release);
    return ndk::ScopedAStatus::ok();
}

::android::status_t StreamUsb::drain(StreamDescriptor::DrainMode) {
    usleep(1000);
    return ::android::OK;
}

::android::status_t StreamUsb::flush() {
    usleep(1000);
    return ::android::OK;
}

::android::status_t StreamUsb::pause() {
    usleep(1000);
    return ::android::OK;
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

StreamInUsb::StreamInUsb(const SinkMetadata& sinkMetadata, StreamContext&& context,
                         const std::vector<MicrophoneInfo>& microphones)
    : StreamUsb(sinkMetadata, std::move(context)), StreamIn(microphones) {}

ndk::ScopedAStatus StreamInUsb::getActiveMicrophones(
        std::vector<MicrophoneDynamicInfo>* _aidl_return __unused) {
    LOG(DEBUG) << __func__ << ": not supported";
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

StreamOutUsb::StreamOutUsb(const SourceMetadata& sourceMetadata, StreamContext&& context,
                           const std::optional<AudioOffloadInfo>& offloadInfo)
    : StreamUsb(sourceMetadata, std::move(context)),
      StreamOut(offloadInfo),
      mChannelCount(getChannelCount(getContext().getChannelLayout())) {}

ndk::ScopedAStatus StreamOutUsb::getHwVolume(std::vector<float>* _aidl_return) {
    *_aidl_return = mHwVolumes;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus StreamOutUsb::setHwVolume(const std::vector<float>& in_channelVolumes) {
    // Avoid using mConnectedDeviceProfiles because it requires a lock.
    for (const auto& device : getConnectedDevices()) {
        if (auto deviceProfile = alsa::getDeviceProfile(device, mIsInput);
            deviceProfile.has_value()) {
            if (auto result = usb::UsbAlsaMixerControl::getInstance().setVolumes(
                        deviceProfile->card, in_channelVolumes);
                !result.isOk()) {
                LOG(ERROR) << __func__
                           << ": failed to set volume for device address=" << *deviceProfile;
                return result;
            }
        }
    }
    mHwVolumes = in_channelVolumes;
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::audio::core
