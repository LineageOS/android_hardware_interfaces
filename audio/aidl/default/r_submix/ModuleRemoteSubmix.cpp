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
#include <error/expected_utils.h>

#include "SubmixRoute.h"
#include "core-impl/ModuleRemoteSubmix.h"
#include "core-impl/StreamRemoteSubmix.h"

using aidl::android::hardware::audio::common::SinkMetadata;
using aidl::android::hardware::audio::common::SourceMetadata;
using aidl::android::media::audio::common::AudioOffloadInfo;
using aidl::android::media::audio::common::AudioPort;
using aidl::android::media::audio::common::AudioPortConfig;
using aidl::android::media::audio::common::MicrophoneInfo;

namespace aidl::android::hardware::audio::core {

ndk::ScopedAStatus ModuleRemoteSubmix::getMicMute(bool* _aidl_return __unused) {
    LOG(DEBUG) << __func__ << ": is not supported";
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus ModuleRemoteSubmix::setMicMute(bool in_mute __unused) {
    LOG(DEBUG) << __func__ << ": is not supported";
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
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

ndk::ScopedAStatus ModuleRemoteSubmix::populateConnectedDevicePort(AudioPort* audioPort) {
    // Find the corresponding mix port and copy its profiles.
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

}  // namespace aidl::android::hardware::audio::core
