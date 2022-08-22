/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <algorithm>
#include <set>

#define LOG_TAG "AHAL_Module"
#define LOG_NDEBUG 0
#include <android-base/logging.h>

#include <aidl/android/media/audio/common/AudioOutputFlags.h>

#include "core-impl/Module.h"
#include "core-impl/utils.h"

using aidl::android::hardware::audio::common::SinkMetadata;
using aidl::android::hardware::audio::common::SourceMetadata;
using aidl::android::media::audio::common::AudioFormatDescription;
using aidl::android::media::audio::common::AudioIoFlags;
using aidl::android::media::audio::common::AudioOffloadInfo;
using aidl::android::media::audio::common::AudioOutputFlags;
using aidl::android::media::audio::common::AudioPort;
using aidl::android::media::audio::common::AudioPortConfig;
using aidl::android::media::audio::common::AudioPortExt;
using aidl::android::media::audio::common::AudioProfile;
using aidl::android::media::audio::common::Int;

namespace aidl::android::hardware::audio::core {

namespace {

bool generateDefaultPortConfig(const AudioPort& port, AudioPortConfig* config) {
    *config = {};
    config->portId = port.id;
    if (port.profiles.empty()) {
        LOG(ERROR) << __func__ << ": port " << port.id << " has no profiles";
        return false;
    }
    const auto& profile = port.profiles.begin();
    config->format = profile->format;
    if (profile->channelMasks.empty()) {
        LOG(ERROR) << __func__ << ": the first profile in port " << port.id
                   << " has no channel masks";
        return false;
    }
    config->channelMask = *profile->channelMasks.begin();
    if (profile->sampleRates.empty()) {
        LOG(ERROR) << __func__ << ": the first profile in port " << port.id
                   << " has no sample rates";
        return false;
    }
    Int sampleRate;
    sampleRate.value = *profile->sampleRates.begin();
    config->sampleRate = sampleRate;
    config->flags = port.flags;
    config->ext = port.ext;
    return true;
}

bool findAudioProfile(const AudioPort& port, const AudioFormatDescription& format,
                      AudioProfile* profile) {
    if (auto profilesIt =
                find_if(port.profiles.begin(), port.profiles.end(),
                        [&format](const auto& profile) { return profile.format == format; });
        profilesIt != port.profiles.end()) {
        *profile = *profilesIt;
        return true;
    }
    return false;
}
}  // namespace

void Module::cleanUpPatch(int32_t patchId) {
    erase_all_values(mPatches, std::set<int32_t>{patchId});
}

void Module::cleanUpPatches(int32_t portConfigId) {
    auto& patches = getConfig().patches;
    if (patches.size() == 0) return;
    auto range = mPatches.equal_range(portConfigId);
    for (auto it = range.first; it != range.second; ++it) {
        auto patchIt = findById<AudioPatch>(patches, it->second);
        if (patchIt != patches.end()) {
            erase_if(patchIt->sourcePortConfigIds,
                     [portConfigId](auto e) { return e == portConfigId; });
            erase_if(patchIt->sinkPortConfigIds,
                     [portConfigId](auto e) { return e == portConfigId; });
        }
    }
    std::set<int32_t> erasedPatches;
    for (size_t i = patches.size() - 1; i != 0; --i) {
        const auto& patch = patches[i];
        if (patch.sourcePortConfigIds.empty() || patch.sinkPortConfigIds.empty()) {
            erasedPatches.insert(patch.id);
            patches.erase(patches.begin() + i);
        }
    }
    erase_all_values(mPatches, erasedPatches);
}

internal::Configuration& Module::getConfig() {
    if (!mConfig) {
        mConfig.reset(new internal::Configuration(internal::getNullPrimaryConfiguration()));
    }
    return *mConfig;
}

void Module::registerPatch(const AudioPatch& patch) {
    auto& configs = getConfig().portConfigs;
    auto do_insert = [&](const std::vector<int32_t>& portConfigIds) {
        for (auto portConfigId : portConfigIds) {
            auto configIt = findById<AudioPortConfig>(configs, portConfigId);
            if (configIt != configs.end()) {
                mPatches.insert(std::pair{portConfigId, patch.id});
                if (configIt->portId != portConfigId) {
                    mPatches.insert(std::pair{configIt->portId, patch.id});
                }
            }
        };
    };
    do_insert(patch.sourcePortConfigIds);
    do_insert(patch.sinkPortConfigIds);
}

ndk::ScopedAStatus Module::getAudioPatches(std::vector<AudioPatch>* _aidl_return) {
    *_aidl_return = getConfig().patches;
    LOG(DEBUG) << __func__ << ": returning " << _aidl_return->size() << " patches";
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Module::getAudioPort(int32_t in_portId, AudioPort* _aidl_return) {
    auto& ports = getConfig().ports;
    auto portIt = findById<AudioPort>(ports, in_portId);
    if (portIt != ports.end()) {
        *_aidl_return = *portIt;
        LOG(DEBUG) << __func__ << ": returning port by id " << in_portId;
        return ndk::ScopedAStatus::ok();
    }
    LOG(ERROR) << __func__ << ": port id " << in_portId << " not found";
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
}

ndk::ScopedAStatus Module::getAudioPortConfigs(std::vector<AudioPortConfig>* _aidl_return) {
    *_aidl_return = getConfig().portConfigs;
    LOG(DEBUG) << __func__ << ": returning " << _aidl_return->size() << " port configs";
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Module::getAudioPorts(std::vector<AudioPort>* _aidl_return) {
    *_aidl_return = getConfig().ports;
    LOG(DEBUG) << __func__ << ": returning " << _aidl_return->size() << " ports";
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Module::getAudioRoutes(std::vector<AudioRoute>* _aidl_return) {
    *_aidl_return = getConfig().routes;
    LOG(DEBUG) << __func__ << ": returning " << _aidl_return->size() << " routes";
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Module::openInputStream(int32_t in_portConfigId,
                                           const SinkMetadata& in_sinkMetadata,
                                           std::shared_ptr<IStreamIn>* _aidl_return) {
    auto& configs = getConfig().portConfigs;
    auto portConfigIt = findById<AudioPortConfig>(configs, in_portConfigId);
    if (portConfigIt == configs.end()) {
        LOG(ERROR) << __func__ << ": existing port config id " << in_portConfigId << " not found";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    const int32_t portId = portConfigIt->portId;
    // In our implementation, configs of mix ports always have unique IDs.
    CHECK(portId != in_portConfigId);
    auto& ports = getConfig().ports;
    auto portIt = findById<AudioPort>(ports, portId);
    if (portIt == ports.end()) {
        LOG(ERROR) << __func__ << ": port id " << portId << " used by port config id "
                   << in_portConfigId << " not found";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    if (portIt->flags.getTag() != AudioIoFlags::Tag::input ||
        portIt->ext.getTag() != AudioPortExt::Tag::mix) {
        LOG(ERROR) << __func__ << ": port config id " << in_portConfigId
                   << " does not correspond to an input mix port";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    if (mStreams.count(in_portConfigId) != 0) {
        LOG(ERROR) << __func__ << ": port config id " << in_portConfigId
                   << " already has a stream opened on it";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    const int32_t maxOpenStreamCount = portIt->ext.get<AudioPortExt::Tag::mix>().maxOpenStreamCount;
    if (maxOpenStreamCount != 0 && mStreams.count(portId) >= maxOpenStreamCount) {
        LOG(ERROR) << __func__ << ": port id " << portId
                   << " has already reached maximum allowed opened stream count: "
                   << maxOpenStreamCount;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    auto stream = ndk::SharedRefBase::make<StreamIn>(in_sinkMetadata);
    mStreams.insert(portId, in_portConfigId, StreamWrapper(stream));
    *_aidl_return = std::move(stream);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Module::openOutputStream(int32_t in_portConfigId,
                                            const SourceMetadata& in_sourceMetadata,
                                            const std::optional<AudioOffloadInfo>& in_offloadInfo,
                                            std::shared_ptr<IStreamOut>* _aidl_return) {
    auto& configs = getConfig().portConfigs;
    auto portConfigIt = findById<AudioPortConfig>(configs, in_portConfigId);
    if (portConfigIt == configs.end()) {
        LOG(ERROR) << __func__ << ": existing port config id " << in_portConfigId << " not found";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    const int32_t portId = portConfigIt->portId;
    // In our implementation, configs of mix ports always have unique IDs.
    CHECK(portId != in_portConfigId);
    auto& ports = getConfig().ports;
    auto portIt = findById<AudioPort>(ports, portId);
    if (portIt == ports.end()) {
        LOG(ERROR) << __func__ << ": port id " << portId << " used by port config id "
                   << in_portConfigId << " not found";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    if (portIt->flags.getTag() != AudioIoFlags::Tag::output ||
        portIt->ext.getTag() != AudioPortExt::Tag::mix) {
        LOG(ERROR) << __func__ << ": port config id " << in_portConfigId
                   << " does not correspond to an output mix port";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    if (mStreams.count(in_portConfigId) != 0) {
        LOG(ERROR) << __func__ << ": port config id " << in_portConfigId
                   << " already has a stream opened on it";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    const int32_t maxOpenStreamCount = portIt->ext.get<AudioPortExt::Tag::mix>().maxOpenStreamCount;
    if (maxOpenStreamCount != 0 && mStreams.count(portId) >= maxOpenStreamCount) {
        LOG(ERROR) << __func__ << ": port id " << portId
                   << " has already reached maximum allowed opened stream count: "
                   << maxOpenStreamCount;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    auto stream = ndk::SharedRefBase::make<StreamOut>(in_sourceMetadata, in_offloadInfo);
    mStreams.insert(portId, in_portConfigId, StreamWrapper(stream));
    *_aidl_return = std::move(stream);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Module::setAudioPatch(const AudioPatch& in_requested, AudioPatch* _aidl_return) {
    if (in_requested.sourcePortConfigIds.empty()) {
        LOG(ERROR) << __func__ << ": requested patch has empty sources list";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    if (!all_unique<int32_t>(in_requested.sourcePortConfigIds)) {
        LOG(ERROR) << __func__ << ": requested patch has duplicate ids in the sources list";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    if (in_requested.sinkPortConfigIds.empty()) {
        LOG(ERROR) << __func__ << ": requested patch has empty sinks list";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    if (!all_unique<int32_t>(in_requested.sinkPortConfigIds)) {
        LOG(ERROR) << __func__ << ": requested patch has duplicate ids in the sinks list";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    auto& configs = getConfig().portConfigs;
    std::vector<int32_t> missingIds;
    auto sources =
            selectByIds<AudioPortConfig>(configs, in_requested.sourcePortConfigIds, &missingIds);
    if (!missingIds.empty()) {
        LOG(ERROR) << __func__ << ": following source port config ids not found: "
                   << ::android::internal::ToString(missingIds);
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    auto sinks = selectByIds<AudioPortConfig>(configs, in_requested.sinkPortConfigIds, &missingIds);
    if (!missingIds.empty()) {
        LOG(ERROR) << __func__ << ": following sink port config ids not found: "
                   << ::android::internal::ToString(missingIds);
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    // bool indicates whether a non-exclusive route is available.
    // If only an exclusive route is available, that means the patch can not be
    // established if there is any other patch which currently uses the sink port.
    std::map<int32_t, bool> allowedSinkPorts;
    auto& routes = getConfig().routes;
    for (auto src : sources) {
        for (const auto& r : routes) {
            const auto& srcs = r.sourcePortIds;
            if (std::find(srcs.begin(), srcs.end(), src->portId) != srcs.end()) {
                if (!allowedSinkPorts[r.sinkPortId]) {  // prefer non-exclusive
                    allowedSinkPorts[r.sinkPortId] = !r.isExclusive;
                }
            }
        }
    }
    for (auto sink : sinks) {
        if (allowedSinkPorts.count(sink->portId) == 0) {
            LOG(ERROR) << __func__ << ": there is no route to the sink port id " << sink->portId;
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }
    }

    auto& patches = getConfig().patches;
    auto existing = patches.end();
    std::optional<decltype(mPatches)> patchesBackup;
    if (in_requested.id != 0) {
        existing = findById<AudioPatch>(patches, in_requested.id);
        if (existing != patches.end()) {
            patchesBackup = mPatches;
            cleanUpPatch(existing->id);
        } else {
            LOG(ERROR) << __func__ << ": not found existing patch id " << in_requested.id;
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }
    }
    // Validate the requested patch.
    for (const auto& [sinkPortId, nonExclusive] : allowedSinkPorts) {
        if (!nonExclusive && mPatches.count(sinkPortId) != 0) {
            LOG(ERROR) << __func__ << ": sink port id " << sinkPortId
                       << "is exclusive and is already used by some other patch";
            if (patchesBackup.has_value()) {
                mPatches = std::move(*patchesBackup);
            }
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
        }
    }
    *_aidl_return = in_requested;
    if (existing == patches.end()) {
        _aidl_return->id = getConfig().nextPatchId++;
        patches.push_back(*_aidl_return);
        existing = patches.begin() + (patches.size() - 1);
    } else {
        *existing = *_aidl_return;
    }
    registerPatch(*existing);
    LOG(DEBUG) << __func__ << ": created or updated patch id " << _aidl_return->id;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Module::setAudioPortConfig(const AudioPortConfig& in_requested,
                                              AudioPortConfig* out_suggested, bool* _aidl_return) {
    LOG(DEBUG) << __func__ << ": requested " << in_requested.toString();
    auto& configs = getConfig().portConfigs;
    auto existing = configs.end();
    if (in_requested.id != 0) {
        if (existing = findById<AudioPortConfig>(configs, in_requested.id);
            existing == configs.end()) {
            LOG(ERROR) << __func__ << ": existing port config id " << in_requested.id
                       << " not found";
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }
    }

    const int portId = existing != configs.end() ? existing->portId : in_requested.portId;
    if (portId == 0) {
        LOG(ERROR) << __func__ << ": input port config does not specify portId";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    auto& ports = getConfig().ports;
    auto portIt = findById<AudioPort>(ports, portId);
    if (portIt == ports.end()) {
        LOG(ERROR) << __func__ << ": input port config points to non-existent portId " << portId;
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    if (existing != configs.end()) {
        *out_suggested = *existing;
    } else {
        AudioPortConfig newConfig;
        if (generateDefaultPortConfig(*portIt, &newConfig)) {
            *out_suggested = newConfig;
        } else {
            LOG(ERROR) << __func__ << ": unable generate a default config for port " << portId;
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }
    }
    // From this moment, 'out_suggested' is either an existing port config,
    // or a new generated config. Now attempt to update it according to the specified
    // fields of 'in_requested'.

    bool requestedIsValid = true, requestedIsFullySpecified = true;

    AudioIoFlags portFlags = portIt->flags;
    if (in_requested.flags.has_value()) {
        if (in_requested.flags.value() != portFlags) {
            LOG(WARNING) << __func__ << ": requested flags "
                         << in_requested.flags.value().toString() << " do not match port's "
                         << portId << " flags " << portFlags.toString();
            requestedIsValid = false;
        }
    } else {
        requestedIsFullySpecified = false;
    }

    AudioProfile portProfile;
    if (in_requested.format.has_value()) {
        const auto& format = in_requested.format.value();
        if (findAudioProfile(*portIt, format, &portProfile)) {
            out_suggested->format = format;
        } else {
            LOG(WARNING) << __func__ << ": requested format " << format.toString()
                         << " is not found in port's " << portId << " profiles";
            requestedIsValid = false;
        }
    } else {
        requestedIsFullySpecified = false;
    }
    if (!findAudioProfile(*portIt, out_suggested->format.value(), &portProfile)) {
        LOG(ERROR) << __func__ << ": port " << portId << " does not support format "
                   << out_suggested->format.value().toString() << " anymore";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    if (in_requested.channelMask.has_value()) {
        const auto& channelMask = in_requested.channelMask.value();
        if (find(portProfile.channelMasks.begin(), portProfile.channelMasks.end(), channelMask) !=
            portProfile.channelMasks.end()) {
            out_suggested->channelMask = channelMask;
        } else {
            LOG(WARNING) << __func__ << ": requested channel mask " << channelMask.toString()
                         << " is not supported for the format " << portProfile.format.toString()
                         << " by the port " << portId;
            requestedIsValid = false;
        }
    } else {
        requestedIsFullySpecified = false;
    }

    if (in_requested.sampleRate.has_value()) {
        const auto& sampleRate = in_requested.sampleRate.value();
        if (find(portProfile.sampleRates.begin(), portProfile.sampleRates.end(),
                 sampleRate.value) != portProfile.sampleRates.end()) {
            out_suggested->sampleRate = sampleRate;
        } else {
            LOG(WARNING) << __func__ << ": requested sample rate " << sampleRate.value
                         << " is not supported for the format " << portProfile.format.toString()
                         << " by the port " << portId;
            requestedIsValid = false;
        }
    } else {
        requestedIsFullySpecified = false;
    }

    if (in_requested.gain.has_value()) {
        // Let's pretend that gain can always be applied.
        out_suggested->gain = in_requested.gain.value();
    }

    if (existing == configs.end() && requestedIsValid && requestedIsFullySpecified) {
        out_suggested->id = getConfig().nextPortId++;
        configs.push_back(*out_suggested);
        *_aidl_return = true;
        LOG(DEBUG) << __func__ << ": created new port config " << out_suggested->toString();
    } else if (existing != configs.end() && requestedIsValid) {
        *existing = *out_suggested;
        *_aidl_return = true;
        LOG(DEBUG) << __func__ << ": updated port config " << out_suggested->toString();
    } else {
        LOG(DEBUG) << __func__ << ": not applied; existing config ? " << (existing != configs.end())
                   << "; requested is valid? " << requestedIsValid << ", fully specified? "
                   << requestedIsFullySpecified;
        *_aidl_return = false;
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Module::resetAudioPatch(int32_t in_patchId) {
    auto& patches = getConfig().patches;
    auto patchIt = findById<AudioPatch>(patches, in_patchId);
    if (patchIt != patches.end()) {
        cleanUpPatch(patchIt->id);
        patches.erase(patchIt);
        LOG(DEBUG) << __func__ << ": erased patch " << in_patchId;
        return ndk::ScopedAStatus::ok();
    }
    LOG(ERROR) << __func__ << ": patch id " << in_patchId << " not found";
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
}

ndk::ScopedAStatus Module::resetAudioPortConfig(int32_t in_portConfigId) {
    auto& configs = getConfig().portConfigs;
    auto configIt = findById<AudioPortConfig>(configs, in_portConfigId);
    if (configIt != configs.end()) {
        if (mStreams.count(in_portConfigId) != 0) {
            LOG(ERROR) << __func__ << ": port config id " << in_portConfigId
                       << " has a stream opened on it";
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
        }
        auto patchIt = mPatches.find(in_portConfigId);
        if (patchIt != mPatches.end()) {
            LOG(ERROR) << __func__ << ": port config id " << in_portConfigId
                       << " is used by the patch with id " << patchIt->second;
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
        }
        auto& initials = getConfig().initialConfigs;
        auto initialIt = findById<AudioPortConfig>(initials, in_portConfigId);
        if (initialIt == initials.end()) {
            configs.erase(configIt);
            LOG(DEBUG) << __func__ << ": erased port config " << in_portConfigId;
        } else if (*configIt != *initialIt) {
            *configIt = *initialIt;
            LOG(DEBUG) << __func__ << ": reset port config " << in_portConfigId;
        }
        return ndk::ScopedAStatus::ok();
    }
    LOG(ERROR) << __func__ << ": port config id " << in_portConfigId << " not found";
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
}

}  // namespace aidl::android::hardware::audio::core
