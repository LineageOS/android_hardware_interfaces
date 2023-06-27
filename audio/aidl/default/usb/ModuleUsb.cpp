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

#define LOG_TAG "AHAL_ModuleUsb"

#include <vector>

#include <Utils.h>
#include <android-base/logging.h>
#include <tinyalsa/asoundlib.h>

#include "UsbAlsaMixerControl.h"
#include "UsbAlsaUtils.h"
#include "core-impl/ModuleUsb.h"
#include "core-impl/StreamUsb.h"

extern "C" {
#include "alsa_device_profile.h"
}

using aidl::android::hardware::audio::common::isUsbInputDeviceType;
using aidl::android::hardware::audio::common::SinkMetadata;
using aidl::android::hardware::audio::common::SourceMetadata;
using aidl::android::media::audio::common::AudioChannelLayout;
using aidl::android::media::audio::common::AudioDeviceAddress;
using aidl::android::media::audio::common::AudioDeviceDescription;
using aidl::android::media::audio::common::AudioDeviceType;
using aidl::android::media::audio::common::AudioFormatDescription;
using aidl::android::media::audio::common::AudioFormatType;
using aidl::android::media::audio::common::AudioOffloadInfo;
using aidl::android::media::audio::common::AudioPort;
using aidl::android::media::audio::common::AudioPortConfig;
using aidl::android::media::audio::common::AudioPortExt;
using aidl::android::media::audio::common::AudioProfile;
using aidl::android::media::audio::common::MicrophoneInfo;

namespace aidl::android::hardware::audio::core {

namespace {

std::vector<AudioChannelLayout> populateChannelMasksFromProfile(const alsa_device_profile* profile,
                                                                bool isInput) {
    std::vector<AudioChannelLayout> channels;
    for (size_t i = 0; i < AUDIO_PORT_MAX_CHANNEL_MASKS && profile->channel_counts[i] != 0; ++i) {
        auto layoutMask =
                usb::getChannelLayoutMaskFromChannelCount(profile->channel_counts[i], isInput);
        if (layoutMask.getTag() == AudioChannelLayout::Tag::layoutMask) {
            channels.push_back(layoutMask);
        }
        auto indexMask = usb::getChannelIndexMaskFromChannelCount(profile->channel_counts[i]);
        if (indexMask.getTag() == AudioChannelLayout::Tag::indexMask) {
            channels.push_back(indexMask);
        }
    }
    return channels;
}

std::vector<int> populateSampleRatesFromProfile(const alsa_device_profile* profile) {
    std::vector<int> sampleRates;
    for (int i = 0; i < std::min(MAX_PROFILE_SAMPLE_RATES, AUDIO_PORT_MAX_SAMPLING_RATES) &&
                    profile->sample_rates[i] != 0;
         i++) {
        sampleRates.push_back(profile->sample_rates[i]);
    }
    return sampleRates;
}

}  // namespace

ndk::ScopedAStatus ModuleUsb::getTelephony(std::shared_ptr<ITelephony>* _aidl_return) {
    *_aidl_return = nullptr;
    LOG(DEBUG) << __func__ << ": returning null";
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ModuleUsb::getBluetooth(std::shared_ptr<IBluetooth>* _aidl_return) {
    *_aidl_return = nullptr;
    LOG(DEBUG) << __func__ << ": returning null";
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ModuleUsb::getMicMute(bool* _aidl_return __unused) {
    LOG(DEBUG) << __func__ << ": is not supported";
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus ModuleUsb::setMicMute(bool in_mute __unused) {
    LOG(DEBUG) << __func__ << ": is not supported";
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus ModuleUsb::createInputStream(const SinkMetadata& sinkMetadata,
                                                StreamContext&& context,
                                                const std::vector<MicrophoneInfo>& microphones,
                                                std::shared_ptr<StreamIn>* result) {
    return createStreamInstance<StreamInUsb>(result, sinkMetadata, std::move(context), microphones);
}

ndk::ScopedAStatus ModuleUsb::createOutputStream(const SourceMetadata& sourceMetadata,
                                                 StreamContext&& context,
                                                 const std::optional<AudioOffloadInfo>& offloadInfo,
                                                 std::shared_ptr<StreamOut>* result) {
    if (offloadInfo.has_value()) {
        LOG(ERROR) << __func__ << ": offload is not supported";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    return createStreamInstance<StreamOutUsb>(result, sourceMetadata, std::move(context),
                                              offloadInfo);
}

ndk::ScopedAStatus ModuleUsb::populateConnectedDevicePort(AudioPort* audioPort) {
    if (audioPort->ext.getTag() != AudioPortExt::Tag::device) {
        LOG(ERROR) << __func__ << ": port id " << audioPort->id << " is not a device port";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    auto& devicePort = audioPort->ext.get<AudioPortExt::Tag::device>();
    if (devicePort.device.type.connection != AudioDeviceDescription::CONNECTION_USB) {
        LOG(ERROR) << __func__ << ": port id " << audioPort->id << " is not a usb device port";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    if (devicePort.device.address.getTag() != AudioDeviceAddress::Tag::alsa) {
        LOG(ERROR) << __func__ << ": port id " << audioPort->id << " is not using alsa address";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    auto& alsaAddress = devicePort.device.address.get<AudioDeviceAddress::Tag::alsa>();
    if (alsaAddress.size() != 2 || alsaAddress[0] < 0 || alsaAddress[1] < 0) {
        LOG(ERROR) << __func__ << ": port id " << audioPort->id << " invalid alsa address";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    const bool isInput = isUsbInputDeviceType(devicePort.device.type.type);
    alsa_device_profile profile;
    profile_init(&profile, isInput ? PCM_IN : PCM_OUT);
    profile.card = alsaAddress[0];
    profile.device = alsaAddress[1];
    if (!profile_read_device_info(&profile)) {
        LOG(ERROR) << __func__ << ": failed to read device info, card=" << profile.card
                   << ", device=" << profile.device;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    std::vector<AudioChannelLayout> channels = populateChannelMasksFromProfile(&profile, isInput);
    std::vector<int> sampleRates = populateSampleRatesFromProfile(&profile);

    for (size_t i = 0; i < std::min(MAX_PROFILE_FORMATS, AUDIO_PORT_MAX_AUDIO_PROFILES) &&
                       profile.formats[i] != PCM_FORMAT_INVALID;
         ++i) {
        auto audioFormatDescription =
                usb::legacy2aidl_pcm_format_AudioFormatDescription(profile.formats[i]);
        if (audioFormatDescription.type == AudioFormatType::DEFAULT) {
            LOG(WARNING) << __func__ << ": unknown pcm type=" << profile.formats[i];
            continue;
        }
        AudioProfile audioProfile = {.format = audioFormatDescription,
                                     .channelMasks = channels,
                                     .sampleRates = sampleRates};
        audioPort->profiles.push_back(std::move(audioProfile));
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ModuleUsb::checkAudioPatchEndpointsMatch(
        const std::vector<AudioPortConfig*>& sources, const std::vector<AudioPortConfig*>& sinks) {
    for (const auto& source : sources) {
        for (const auto& sink : sinks) {
            if (source->sampleRate != sink->sampleRate ||
                source->channelMask != sink->channelMask || source->format != sink->format) {
                LOG(ERROR) << __func__
                           << ": mismatch port configuration, source=" << source->toString()
                           << ", sink=" << sink->toString();
                return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
            }
        }
    }
    return ndk::ScopedAStatus::ok();
}

void ModuleUsb::onExternalDeviceConnectionChanged(
        const ::aidl::android::media::audio::common::AudioPort& audioPort, bool connected) {
    if (audioPort.ext.getTag() != AudioPortExt::Tag::device) {
        return;
    }
    const auto& address = audioPort.ext.get<AudioPortExt::Tag::device>().device.address;
    if (address.getTag() != AudioDeviceAddress::alsa) {
        return;
    }
    const int card = address.get<AudioDeviceAddress::alsa>()[0];
    usb::UsbAlsaMixerControl::getInstance().setDeviceConnectionState(card, getMasterMute(),
                                                                     getMasterVolume(), connected);
}

ndk::ScopedAStatus ModuleUsb::onMasterMuteChanged(bool mute) {
    return usb::UsbAlsaMixerControl::getInstance().setMasterMute(mute);
}

ndk::ScopedAStatus ModuleUsb::onMasterVolumeChanged(float volume) {
    return usb::UsbAlsaMixerControl::getInstance().setMasterVolume(volume);
}

}  // namespace aidl::android::hardware::audio::core
