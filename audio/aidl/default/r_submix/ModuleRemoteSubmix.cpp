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

#include <stdio.h>
#include <vector>

#include <android-base/logging.h>
#include <error/expected_utils.h>

#include "SubmixRoute.h"
#include "core-impl/ModuleRemoteSubmix.h"
#include "core-impl/StreamRemoteSubmix.h"

using aidl::android::hardware::audio::common::SinkMetadata;
using aidl::android::hardware::audio::common::SourceMetadata;
using aidl::android::media::audio::common::AudioDeviceAddress;
using aidl::android::media::audio::common::AudioFormatType;
using aidl::android::media::audio::common::AudioIoFlags;
using aidl::android::media::audio::common::AudioOffloadInfo;
using aidl::android::media::audio::common::AudioPort;
using aidl::android::media::audio::common::AudioPortConfig;
using aidl::android::media::audio::common::AudioPortExt;
using aidl::android::media::audio::common::AudioProfile;
using aidl::android::media::audio::common::Int;
using aidl::android::media::audio::common::MicrophoneInfo;

namespace aidl::android::hardware::audio::core {

namespace {

std::optional<r_submix::AudioConfig> getRemoteEndConfig(const AudioPort& audioPort) {
    const auto& deviceAddress = audioPort.ext.get<AudioPortExt::device>().device.address;
    const bool isInput = audioPort.flags.getTag() == AudioIoFlags::input;
    if (auto submixRoute = r_submix::SubmixRoute::findRoute(deviceAddress);
        submixRoute != nullptr) {
        if ((isInput && submixRoute->isStreamOutOpen()) ||
            (!isInput && submixRoute->isStreamInOpen())) {
            return submixRoute->getPipeConfig();
        }
    }
    return {};
}

}  // namespace

ndk::ScopedAStatus ModuleRemoteSubmix::getMicMute(bool* _aidl_return __unused) {
    LOG(DEBUG) << __func__ << ": is not supported";
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus ModuleRemoteSubmix::setMicMute(bool in_mute __unused) {
    LOG(DEBUG) << __func__ << ": is not supported";
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus ModuleRemoteSubmix::setAudioPortConfig(const AudioPortConfig& in_requested,
                                                          AudioPortConfig* out_suggested,
                                                          bool* _aidl_return) {
    auto fillConfig = [this](const AudioPort& port, AudioPortConfig* config) {
        if (port.ext.getTag() == AudioPortExt::device) {
            if (auto pipeConfig = getRemoteEndConfig(port); pipeConfig.has_value()) {
                LOG(DEBUG) << "setAudioPortConfig: suggesting port config from the remote end.";
                config->format = pipeConfig->format;
                config->channelMask = pipeConfig->channelLayout;
                config->sampleRate = Int{.value = pipeConfig->sampleRate};
                config->flags = port.flags;
                config->ext = port.ext;
                return true;
            }
        }
        return generateDefaultPortConfig(port, config);
    };
    return Module::setAudioPortConfigImpl(in_requested, fillConfig, out_suggested, _aidl_return);
}

ndk::ScopedAStatus ModuleRemoteSubmix::createInputStream(
        StreamContext&& context, const SinkMetadata& sinkMetadata,
        const std::vector<MicrophoneInfo>& microphones, std::shared_ptr<StreamIn>* result) {
    return createStreamInstance<StreamInRemoteSubmix>(result, std::move(context), sinkMetadata,
                                                      microphones);
}

ndk::ScopedAStatus ModuleRemoteSubmix::createOutputStream(
        StreamContext&& context, const SourceMetadata& sourceMetadata,
        const std::optional<AudioOffloadInfo>& offloadInfo, std::shared_ptr<StreamOut>* result) {
    return createStreamInstance<StreamOutRemoteSubmix>(result, std::move(context), sourceMetadata,
                                                       offloadInfo);
}

ndk::ScopedAStatus ModuleRemoteSubmix::populateConnectedDevicePort(AudioPort* audioPort, int32_t) {
    if (audioPort->ext.getTag() != AudioPortExt::device) {
        LOG(ERROR) << __func__ << ": not a device port: " << audioPort->toString();
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    // If there is already a pipe with a stream for the port address, provide its configuration as
    // the only option. Otherwise, find the corresponding mix port and copy its profiles.
    if (auto pipeConfig = getRemoteEndConfig(*audioPort); pipeConfig.has_value()) {
        audioPort->profiles.clear();
        audioPort->profiles.push_back(AudioProfile{
                .format = pipeConfig->format,
                .channelMasks = std::vector<AudioChannelLayout>({pipeConfig->channelLayout}),
                .sampleRates = std::vector<int>({pipeConfig->sampleRate})});
        LOG(DEBUG) << __func__ << ": populated from remote end as: " << audioPort->toString();
        return ndk::ScopedAStatus::ok();
    }

    // At this moment, the port has the same ID as the template port, see connectExternalDevice.
    std::vector<AudioRoute*> routes = getAudioRoutesForAudioPortImpl(audioPort->id);
    if (routes.empty()) {
        LOG(ERROR) << __func__ << ": no routes found for the port " << audioPort->toString();
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    const auto& route = *routes.begin();
    AudioPort mixPort;
    if (route->sinkPortId == audioPort->id) {
        if (route->sourcePortIds.empty()) {
            LOG(ERROR) << __func__ << ": invalid route " << route->toString();
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }
        RETURN_STATUS_IF_ERROR(getAudioPort(*route->sourcePortIds.begin(), &mixPort));
    } else {
        RETURN_STATUS_IF_ERROR(getAudioPort(route->sinkPortId, &mixPort));
    }
    audioPort->profiles = mixPort.profiles;
    LOG(DEBUG) << __func__ << ": populated from the mix port as: " << audioPort->toString();
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

ndk::ScopedAStatus ModuleRemoteSubmix::onMasterMuteChanged(bool __unused) {
    LOG(DEBUG) << __func__ << ": is not supported";
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus ModuleRemoteSubmix::onMasterVolumeChanged(float __unused) {
    LOG(DEBUG) << __func__ << ": is not supported";
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

int32_t ModuleRemoteSubmix::getNominalLatencyMs(const AudioPortConfig&) {
    // See the note on kDefaultPipePeriodCount.
    static constexpr int32_t kMaxLatencyMs =
            (r_submix::kDefaultPipeSizeInFrames * 1000) / r_submix::kDefaultSampleRateHz;
    static constexpr int32_t kMinLatencyMs = kMaxLatencyMs / r_submix::kDefaultPipePeriodCount;
    return kMinLatencyMs;
}

binder_status_t ModuleRemoteSubmix::dump(int fd, const char** /*args*/, uint32_t /*numArgs*/) {
    dprintf(fd, "\nSubmixRoutes:\n%s\n", r_submix::SubmixRoute::dumpRoutes().c_str());
    return STATUS_OK;
}

}  // namespace aidl::android::hardware::audio::core
