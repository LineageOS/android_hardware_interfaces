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
      mFrameSizeBytes(getContext().getFrameSize()),
      mIsInput(isInput(metadata)),
      mConfig(maybePopulateConfig(getContext(), mIsInput)) {}

// static
std::optional<struct pcm_config> StreamUsb::maybePopulateConfig(const StreamContext& context,
                                                                bool isInput) {
    struct pcm_config config;
    config.channels = usb::getChannelCountFromChannelMask(context.getChannelLayout(), isInput);
    if (config.channels == 0) {
        LOG(ERROR) << __func__ << ": invalid channel=" << context.getChannelLayout().toString();
        return std::nullopt;
    }
    config.format = usb::aidl2legacy_AudioFormatDescription_pcm_format(context.getFormat());
    if (config.format == PCM_FORMAT_INVALID) {
        LOG(ERROR) << __func__ << ": invalid format=" << context.getFormat().toString();
        return std::nullopt;
    }
    config.rate = context.getSampleRate();
    if (config.rate == 0) {
        LOG(ERROR) << __func__ << ": invalid sample rate=" << config.rate;
        return std::nullopt;
    }
    return config;
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
    RETURN_STATUS_IF_ERROR(StreamCommonImpl::setConnectedDevices(connectedDevices));
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
        // 'setConnectedDevices' has been called. I/O will be restarted.
        *actualFrameCount = 0;
        *latencyMs = StreamDescriptor::LATENCY_UNKNOWN;
        return ::android::OK;
    }
    const size_t bytesToTransfer = frameCount * mFrameSizeBytes;
    unsigned maxLatency = 0;
    if (mIsInput) {
        if (mAlsaDeviceProxies.empty()) {
            LOG(FATAL) << __func__ << ": no input devices";
            return ::android::NO_INIT;
        }
        // For input case, only support single device.
        proxy_read(mAlsaDeviceProxies[0].get(), buffer, bytesToTransfer);
        maxLatency = proxy_get_latency(mAlsaDeviceProxies[0].get());
    } else {
        for (auto& proxy : mAlsaDeviceProxies) {
            proxy_write(proxy.get(), buffer, bytesToTransfer);
            maxLatency = std::max(maxLatency, proxy_get_latency(proxy.get()));
        }
    }
    *actualFrameCount = frameCount;
    maxLatency = std::min(maxLatency, static_cast<unsigned>(std::numeric_limits<int32_t>::max()));
    *latencyMs = maxLatency;
    return ::android::OK;
}

::android::status_t StreamUsb::standby() {
    mAlsaDeviceProxies.clear();
    return ::android::OK;
}

void StreamUsb::shutdown() {
    mAlsaDeviceProxies.clear();
}

::android::status_t StreamUsb::start() {
    std::vector<AudioDeviceAddress> connectedDevices;
    {
        std::lock_guard guard(mLock);
        std::transform(mConnectedDevices.begin(), mConnectedDevices.end(),
                       std::back_inserter(connectedDevices),
                       [](const auto& device) { return device.address; });
        mConnectedDevicesUpdated.store(false, std::memory_order_release);
    }
    decltype(mAlsaDeviceProxies) alsaDeviceProxies;
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

        AlsaDeviceProxy proxy(new alsa_device_proxy, [](alsa_device_proxy* proxy) {
            proxy_close(proxy);
            free(proxy);
        });
        // Always ask for alsa configure as required since the configuration should be supported
        // by the connected device. That is guaranteed by `setAudioPortConfig` and
        // `setAudioPatch`.
        if (int err = proxy_prepare(proxy.get(), &profile,
                                    const_cast<struct pcm_config*>(&mConfig.value()),
                                    true /*is_bit_perfect*/);
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
    mAlsaDeviceProxies = std::move(alsaDeviceProxies);
    return ::android::OK;
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
