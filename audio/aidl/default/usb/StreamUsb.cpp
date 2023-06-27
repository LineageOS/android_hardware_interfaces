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

#define LOG_TAG "AHAL_StreamUsb"
#include <android-base/logging.h>

#include <Utils.h>
#include <error/expected_utils.h>

#include "UsbAlsaMixerControl.h"
#include "UsbAlsaUtils.h"
#include "core-impl/Module.h"
#include "core-impl/StreamUsb.h"

extern "C" {
#include "alsa_device_profile.h"
}

using aidl::android::hardware::audio::common::getChannelCount;
using aidl::android::hardware::audio::common::SinkMetadata;
using aidl::android::hardware::audio::common::SourceMetadata;
using aidl::android::media::audio::common::AudioDevice;
using aidl::android::media::audio::common::AudioDeviceAddress;
using aidl::android::media::audio::common::AudioOffloadInfo;
using aidl::android::media::audio::common::AudioPortExt;
using aidl::android::media::audio::common::MicrophoneDynamicInfo;
using aidl::android::media::audio::common::MicrophoneInfo;
using android::OK;
using android::status_t;

namespace aidl::android::hardware::audio::core {

StreamUsb::StreamUsb(const Metadata& metadata, StreamContext&& context)
    : StreamCommonImpl(metadata, std::move(context)),
      mFrameSizeBytes(context.getFrameSize()),
      mIsInput(isInput(metadata)) {
    struct pcm_config config;
    config.channels = usb::getChannelCountFromChannelMask(context.getChannelLayout(), mIsInput);
    if (config.channels == 0) {
        LOG(ERROR) << __func__ << ": invalid channel=" << context.getChannelLayout().toString();
        return;
    }
    config.format = usb::aidl2legacy_AudioFormatDescription_pcm_format(context.getFormat());
    if (config.format == PCM_FORMAT_INVALID) {
        LOG(ERROR) << __func__ << ": invalid format=" << context.getFormat().toString();
        return;
    }
    config.rate = context.getSampleRate();
    if (config.rate == 0) {
        LOG(ERROR) << __func__ << ": invalid sample rate=" << config.rate;
        return;
    }
    mConfig = config;
}

::android::status_t StreamUsb::init() {
    return mConfig.has_value() ? ::android::OK : ::android::NO_INIT;
}

const StreamCommonInterface::ConnectedDevices& StreamUsb::getConnectedDevices() const {
    std::lock_guard guard(mLock);
    return mConnectedDevices;
}

ndk::ScopedAStatus StreamUsb::setConnectedDevices(
        const std::vector<AudioDevice>& connectedDevices) {
    if (mIsInput && connectedDevices.size() > 1) {
        LOG(ERROR) << __func__ << ": wrong device size(" << connectedDevices.size()
                   << ") for input stream";
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    for (const auto& connectedDevice : connectedDevices) {
        if (connectedDevice.address.getTag() != AudioDeviceAddress::alsa) {
            LOG(ERROR) << __func__ << ": bad device address" << connectedDevice.address.toString();
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }
    }
    std::lock_guard guard(mLock);
    mAlsaDeviceProxies.clear();
    RETURN_STATUS_IF_ERROR(StreamCommonImpl::setConnectedDevices(connectedDevices));
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
    {
        std::lock_guard guard(mLock);
        if (!mConfig.has_value() || mConnectedDevices.empty()) {
            LOG(ERROR) << __func__ << ": failed, has config: " << mConfig.has_value()
                       << ", has connected devices: " << mConnectedDevices.empty();
            return ::android::NO_INIT;
        }
    }
    if (mIsStandby) {
        if (::android::status_t status = exitStandby(); status != ::android::OK) {
            LOG(ERROR) << __func__ << ": failed to exit standby, status=" << status;
            return status;
        }
    }
    std::vector<std::shared_ptr<alsa_device_proxy>> alsaDeviceProxies;
    {
        std::lock_guard guard(mLock);
        alsaDeviceProxies = mAlsaDeviceProxies;
    }
    const size_t bytesToTransfer = frameCount * mFrameSizeBytes;
    if (mIsInput) {
        // For input case, only support single device.
        proxy_read(alsaDeviceProxies[0].get(), buffer, bytesToTransfer);
    } else {
        for (auto& proxy : alsaDeviceProxies) {
            proxy_write(proxy.get(), buffer, bytesToTransfer);
        }
    }
    *actualFrameCount = frameCount;
    *latencyMs = Module::kLatencyMs;
    return ::android::OK;
}

::android::status_t StreamUsb::standby() {
    if (!mIsStandby) {
        std::lock_guard guard(mLock);
        mAlsaDeviceProxies.clear();
        mIsStandby = true;
    }
    return ::android::OK;
}

void StreamUsb::shutdown() {}

::android::status_t StreamUsb::exitStandby() {
    std::vector<AudioDeviceAddress> connectedDevices;
    {
        std::lock_guard guard(mLock);
        std::transform(mConnectedDevices.begin(), mConnectedDevices.end(),
                       std::back_inserter(connectedDevices),
                       [](const auto& device) { return device.address; });
    }
    std::vector<std::shared_ptr<alsa_device_proxy>> alsaDeviceProxies;
    for (const auto& device : connectedDevices) {
        alsa_device_profile profile;
        profile_init(&profile, mIsInput ? PCM_IN : PCM_OUT);
        profile.card = device.get<AudioDeviceAddress::alsa>()[0];
        profile.device = device.get<AudioDeviceAddress::alsa>()[1];
        if (!profile_read_device_info(&profile)) {
            LOG(ERROR) << __func__
                       << ": unable to read device info, device address=" << device.toString();
            return ::android::UNKNOWN_ERROR;
        }

        auto proxy = std::shared_ptr<alsa_device_proxy>(new alsa_device_proxy(),
                                                        [](alsa_device_proxy* proxy) {
                                                            proxy_close(proxy);
                                                            free(proxy);
                                                        });
        // Always ask for alsa configure as required since the configuration should be supported
        // by the connected device. That is guaranteed by `setAudioPortConfig` and
        // `setAudioPatch`.
        if (int err =
                    proxy_prepare(proxy.get(), &profile, &mConfig.value(), true /*is_bit_perfect*/);
            err != 0) {
            LOG(ERROR) << __func__ << ": fail to prepare for device address=" << device.toString()
                       << " error=" << err;
            return ::android::UNKNOWN_ERROR;
        }
        if (int err = proxy_open(proxy.get()); err != 0) {
            LOG(ERROR) << __func__ << ": failed to open device, address=" << device.toString()
                       << " error=" << err;
            return ::android::UNKNOWN_ERROR;
        }
        alsaDeviceProxies.push_back(std::move(proxy));
    }
    {
        std::lock_guard guard(mLock);
        mAlsaDeviceProxies = alsaDeviceProxies;
    }
    mIsStandby = false;
    return ::android::OK;
}

// static
ndk::ScopedAStatus StreamInUsb::createInstance(const SinkMetadata& sinkMetadata,
                                               StreamContext&& context,
                                               const std::vector<MicrophoneInfo>& microphones,
                                               std::shared_ptr<StreamIn>* result) {
    std::shared_ptr<StreamIn> stream =
            ndk::SharedRefBase::make<StreamInUsb>(sinkMetadata, std::move(context), microphones);
    if (auto status = stream->initInstance(stream); !status.isOk()) {
        return status;
    }
    *result = std::move(stream);
    return ndk::ScopedAStatus::ok();
}

StreamInUsb::StreamInUsb(const SinkMetadata& sinkMetadata, StreamContext&& context,
                         const std::vector<MicrophoneInfo>& microphones)
    : StreamUsb(sinkMetadata, std::move(context)), StreamIn(microphones) {}

ndk::ScopedAStatus StreamInUsb::getActiveMicrophones(
        std::vector<MicrophoneDynamicInfo>* _aidl_return __unused) {
    LOG(DEBUG) << __func__ << ": not supported";
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

// static
ndk::ScopedAStatus StreamOutUsb::createInstance(const SourceMetadata& sourceMetadata,
                                                StreamContext&& context,
                                                const std::optional<AudioOffloadInfo>& offloadInfo,
                                                std::shared_ptr<StreamOut>* result) {
    if (offloadInfo.has_value()) {
        LOG(ERROR) << __func__ << ": offload is not supported";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    std::shared_ptr<StreamOut> stream =
            ndk::SharedRefBase::make<StreamOutUsb>(sourceMetadata, std::move(context), offloadInfo);
    if (auto status = stream->initInstance(stream); !status.isOk()) {
        return status;
    }
    *result = std::move(stream);
    return ndk::ScopedAStatus::ok();
}

StreamOutUsb::StreamOutUsb(const SourceMetadata& sourceMetadata, StreamContext&& context,
                           const std::optional<AudioOffloadInfo>& offloadInfo)
    : StreamUsb(sourceMetadata, std::move(context)), StreamOut(offloadInfo) {
    mChannelCount = getChannelCount(getContext().getChannelLayout());
}

ndk::ScopedAStatus StreamOutUsb::getHwVolume(std::vector<float>* _aidl_return) {
    *_aidl_return = mHwVolumes;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus StreamOutUsb::setHwVolume(const std::vector<float>& in_channelVolumes) {
    for (const auto& device : getConnectedDevices()) {
        if (device.address.getTag() != AudioDeviceAddress::alsa) {
            LOG(DEBUG) << __func__ << ": skip as the device address is not alsa";
            continue;
        }
        const int card = device.address.get<AudioDeviceAddress::alsa>()[0];
        if (auto result =
                    usb::UsbAlsaMixerControl::getInstance().setVolumes(card, in_channelVolumes);
            !result.isOk()) {
            LOG(ERROR) << __func__ << ": failed to set volume for device, card=" << card;
            return result;
        }
    }
    mHwVolumes = in_channelVolumes;
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::audio::core
