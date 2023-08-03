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

#include <vector>

#define LOG_TAG "AHAL_ModuleUsb"
#include <Utils.h>
#include <android-base/logging.h>

#include "UsbAlsaMixerControl.h"
#include "alsa/Utils.h"
#include "core-impl/ModuleUsb.h"
#include "core-impl/StreamUsb.h"

using aidl::android::hardware::audio::common::SinkMetadata;
using aidl::android::hardware::audio::common::SourceMetadata;
using aidl::android::media::audio::common::AudioDeviceDescription;
using aidl::android::media::audio::common::AudioOffloadInfo;
using aidl::android::media::audio::common::AudioPort;
using aidl::android::media::audio::common::AudioPortConfig;
using aidl::android::media::audio::common::AudioPortExt;
using aidl::android::media::audio::common::MicrophoneInfo;

namespace aidl::android::hardware::audio::core {

namespace {

bool isUsbDevicePort(const AudioPort& audioPort) {
    return audioPort.ext.getTag() == AudioPortExt::Tag::device &&
           audioPort.ext.get<AudioPortExt::Tag::device>().device.type.connection ==
                   AudioDeviceDescription::CONNECTION_USB;
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

ndk::ScopedAStatus ModuleUsb::createInputStream(StreamContext&& context,
                                                const SinkMetadata& sinkMetadata,
                                                const std::vector<MicrophoneInfo>& microphones,
                                                std::shared_ptr<StreamIn>* result) {
    return createStreamInstance<StreamInUsb>(result, std::move(context), sinkMetadata, microphones);
}

ndk::ScopedAStatus ModuleUsb::createOutputStream(StreamContext&& context,
                                                 const SourceMetadata& sourceMetadata,
                                                 const std::optional<AudioOffloadInfo>& offloadInfo,
                                                 std::shared_ptr<StreamOut>* result) {
    if (offloadInfo.has_value()) {
        LOG(ERROR) << __func__ << ": offload is not supported";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    return createStreamInstance<StreamOutUsb>(result, std::move(context), sourceMetadata,
                                              offloadInfo);
}

ndk::ScopedAStatus ModuleUsb::populateConnectedDevicePort(AudioPort* audioPort) {
    if (!isUsbDevicePort(*audioPort)) {
        LOG(ERROR) << __func__ << ": port id " << audioPort->id << " is not a usb device port";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    return ModuleAlsa::populateConnectedDevicePort(audioPort);
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
    if (!isUsbDevicePort(audioPort)) {
        return;
    }
    auto profile = alsa::getDeviceProfile(audioPort);
    if (!profile.has_value()) {
        return;
    }
    usb::UsbAlsaMixerControl::getInstance().setDeviceConnectionState(profile->card, getMasterMute(),
                                                                     getMasterVolume(), connected);
}

ndk::ScopedAStatus ModuleUsb::onMasterMuteChanged(bool mute) {
    return usb::UsbAlsaMixerControl::getInstance().setMasterMute(mute);
}

ndk::ScopedAStatus ModuleUsb::onMasterVolumeChanged(float volume) {
    return usb::UsbAlsaMixerControl::getInstance().setMasterVolume(volume);
}

}  // namespace aidl::android::hardware::audio::core
