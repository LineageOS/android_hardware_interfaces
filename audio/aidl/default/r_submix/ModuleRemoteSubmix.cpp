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

#define LOG_TAG "AHAL_ModuleRemoteSubmix"

#include <vector>

#include <android-base/logging.h>

#include "RemoteSubmixUtils.h"
#include "core-impl/ModuleRemoteSubmix.h"
#include "core-impl/StreamRemoteSubmix.h"

using aidl::android::hardware::audio::common::SinkMetadata;
using aidl::android::hardware::audio::common::SourceMetadata;
using aidl::android::media::audio::common::AudioOffloadInfo;
using aidl::android::media::audio::common::AudioPort;
using aidl::android::media::audio::common::AudioPortConfig;
using aidl::android::media::audio::common::MicrophoneInfo;

namespace aidl::android::hardware::audio::core {

ndk::ScopedAStatus ModuleRemoteSubmix::getTelephony(std::shared_ptr<ITelephony>* _aidl_return) {
    *_aidl_return = nullptr;
    LOG(DEBUG) << __func__ << ": returning null";
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ModuleRemoteSubmix::getBluetooth(std::shared_ptr<IBluetooth>* _aidl_return) {
    *_aidl_return = nullptr;
    LOG(DEBUG) << __func__ << ": returning null";
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ModuleRemoteSubmix::getMicMute(bool* _aidl_return __unused) {
    LOG(DEBUG) << __func__ << ": is not supported";
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus ModuleRemoteSubmix::setMicMute(bool in_mute __unused) {
    LOG(DEBUG) << __func__ << ": is not supported";
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus ModuleRemoteSubmix::createInputStream(
        const SinkMetadata& sinkMetadata, StreamContext&& context,
        const std::vector<MicrophoneInfo>& microphones, std::shared_ptr<StreamIn>* result) {
    return createStreamInstance<StreamInRemoteSubmix>(result, sinkMetadata, std::move(context),
                                                      microphones);
}

ndk::ScopedAStatus ModuleRemoteSubmix::createOutputStream(
        const SourceMetadata& sourceMetadata, StreamContext&& context,
        const std::optional<AudioOffloadInfo>& offloadInfo, std::shared_ptr<StreamOut>* result) {
    return createStreamInstance<StreamOutRemoteSubmix>(result, sourceMetadata, std::move(context),
                                                       offloadInfo);
}

ndk::ScopedAStatus ModuleRemoteSubmix::populateConnectedDevicePort(AudioPort* audioPort) {
    LOG(VERBOSE) << __func__ << ": Profiles already populated by Configuration";
    for (auto profile : audioPort->profiles) {
        for (auto channelMask : profile.channelMasks) {
            if (!r_submix::isChannelMaskSupported(channelMask)) {
                LOG(ERROR) << __func__ << ": the profile " << profile.name
                           << " has unsupported channel mask : " << channelMask.toString();
                return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
            }
        }
        for (auto sampleRate : profile.sampleRates) {
            if (!r_submix::isSampleRateSupported(sampleRate)) {
                LOG(ERROR) << __func__ << ": the profile " << profile.name
                           << " has unsupported sample rate : " << sampleRate;
                return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
            }
        }
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus ModuleRemoteSubmix::checkAudioPatchEndpointsMatch(
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

void ModuleRemoteSubmix::onExternalDeviceConnectionChanged(
        const ::aidl::android::media::audio::common::AudioPort& audioPort __unused,
        bool connected __unused) {
    LOG(DEBUG) << __func__ << ": do nothing and return";
}

ndk::ScopedAStatus ModuleRemoteSubmix::onMasterMuteChanged(bool __unused) {
    LOG(DEBUG) << __func__ << ": is not supported";
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus ModuleRemoteSubmix::onMasterVolumeChanged(float __unused) {
    LOG(DEBUG) << __func__ << ": is not supported";
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

}  // namespace aidl::android::hardware::audio::core
