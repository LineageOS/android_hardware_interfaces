/*
 * Copyright (C) 2020 The Android Open Source Project
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

#define LOG_TAG "AudioControl"
// #define LOG_NDEBUG 0

#include "AudioControl.h"

#include <aidl/android/hardware/automotive/audiocontrol/AudioFocusChange.h>
#include <aidl/android/hardware/automotive/audiocontrol/DuckingInfo.h>
#include <aidl/android/hardware/automotive/audiocontrol/IFocusListener.h>

#include <android-base/logging.h>
#include <android-base/parseint.h>
#include <android-base/strings.h>

#include <android_audio_policy_configuration_V7_0-enums.h>
#include <private/android_filesystem_config.h>

#include <numeric>

#include <stdio.h>

namespace aidl::android::hardware::automotive::audiocontrol {

using ::android::base::EqualsIgnoreCase;
using ::android::base::ParseInt;
using ::std::shared_ptr;
using ::std::string;

namespace xsd {
using namespace ::android::audio::policy::configuration::V7_0;
}

namespace {
const float kLowerBound = -1.0f;
const float kUpperBound = 1.0f;
bool checkCallerHasWritePermissions(int fd) {
    // Double check that's only called by root - it should be be blocked at debug() level,
    // but it doesn't hurt to make sure...
    if (AIBinder_getCallingUid() != AID_ROOT) {
        dprintf(fd, "Must be root\n");
        return false;
    }
    return true;
}

bool isValidValue(float value) {
    return (value >= kLowerBound) && (value <= kUpperBound);
}

bool safelyParseInt(string s, int* out) {
    if (!ParseInt(s, out)) {
        return false;
    }
    return true;
}
}  // namespace

ndk::ScopedAStatus AudioControl::registerFocusListener(
        const shared_ptr<IFocusListener>& in_listener) {
    LOG(DEBUG) << "registering focus listener";

    if (in_listener) {
        std::atomic_store(&mFocusListener, in_listener);
    } else {
        LOG(ERROR) << "Unexpected nullptr for listener resulting in no-op.";
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus AudioControl::setBalanceTowardRight(float value) {
    if (isValidValue(value)) {
        // Just log in this default mock implementation
        LOG(INFO) << "Balance set to " << value;
        return ndk::ScopedAStatus::ok();
    }

    LOG(ERROR) << "Balance value out of range -1 to 1 at " << value;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus AudioControl::setFadeTowardFront(float value) {
    if (isValidValue(value)) {
        // Just log in this default mock implementation
        LOG(INFO) << "Fader set to " << value;
        return ndk::ScopedAStatus::ok();
    }

    LOG(ERROR) << "Fader value out of range -1 to 1 at " << value;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus AudioControl::onAudioFocusChange(const string& in_usage, int32_t in_zoneId,
                                                    AudioFocusChange in_focusChange) {
    LOG(INFO) << "Focus changed: " << toString(in_focusChange).c_str() << " for usage "
              << in_usage.c_str() << " in zone " << in_zoneId;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus AudioControl::onDevicesToDuckChange(
        const std::vector<DuckingInfo>& in_duckingInfos) {
    LOG(INFO) << "AudioControl::onDevicesToDuckChange";
    for (const DuckingInfo& duckingInfo : in_duckingInfos) {
        LOG(INFO) << "zone: " << duckingInfo.zoneId;
        LOG(INFO) << "Devices to duck:";
        for (const auto& addressToDuck : duckingInfo.deviceAddressesToDuck) {
            LOG(INFO) << addressToDuck;
        }
        LOG(INFO) << "Devices to unduck:";
        for (const auto& addressToUnduck : duckingInfo.deviceAddressesToUnduck) {
            LOG(INFO) << addressToUnduck;
        }
        LOG(INFO) << "Usages holding focus:";
        for (const auto& usage : duckingInfo.usagesHoldingFocus) {
            LOG(INFO) << usage;
        }
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus AudioControl::onDevicesToMuteChange(
        const std::vector<MutingInfo>& in_mutingInfos) {
    LOG(INFO) << "AudioControl::onDevicesToMuteChange";
    for (const MutingInfo& mutingInfo : in_mutingInfos) {
        LOG(INFO) << "zone: " << mutingInfo.zoneId;
        LOG(INFO) << "Devices to mute:";
        for (const auto& addressToMute : mutingInfo.deviceAddressesToMute) {
            LOG(INFO) << addressToMute;
        }
        LOG(INFO) << "Devices to unmute:";
        for (const auto& addressToUnmute : mutingInfo.deviceAddressesToUnmute) {
            LOG(INFO) << addressToUnmute;
        }
    }
    return ndk::ScopedAStatus::ok();
}

template <typename aidl_type>
static inline std::string toString(const std::vector<aidl_type>& in_values) {
    return std::accumulate(std::begin(in_values), std::end(in_values), std::string{},
                           [](std::string& ls, const aidl_type& rs) {
                               return ls += (ls.empty() ? "" : ",") + rs.toString();
                           });
}
template <typename aidl_enum_type>
static inline std::string toEnumString(const std::vector<aidl_enum_type>& in_values) {
    return std::accumulate(std::begin(in_values), std::end(in_values), std::string{},
                           [](std::string& ls, const aidl_enum_type& rs) {
                               return ls += (ls.empty() ? "" : ",") + toString(rs);
                           });
}

ndk::ScopedAStatus AudioControl::onAudioFocusChangeWithMetaData(
        const audiohalcommon::PlaybackTrackMetadata& in_playbackMetaData, int32_t in_zoneId,
        AudioFocusChange in_focusChange) {
    LOG(INFO) << "Focus changed: " << toString(in_focusChange).c_str() << " for metadata "
              << in_playbackMetaData.toString().c_str() << " in zone " << in_zoneId;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus AudioControl::setAudioDeviceGainsChanged(
        const std::vector<Reasons>& in_reasons, const std::vector<AudioGainConfigInfo>& in_gains) {
    LOG(INFO) << "Audio Device Gains changed: resons:" << toEnumString(in_reasons).c_str()
              << " for devices: " << toString(in_gains).c_str();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus AudioControl::registerGainCallback(
        const std::shared_ptr<IAudioGainCallback>& in_callback) {
    LOG(DEBUG) << ": " << __func__;
    if (in_callback) {
        std::atomic_store(&mAudioGainCallback, in_callback);
    } else {
        LOG(ERROR) << "Unexpected nullptr for audio gain callback resulting in no-op.";
    }
    return ndk::ScopedAStatus::ok();
}

binder_status_t AudioControl::dump(int fd, const char** args, uint32_t numArgs) {
    if (numArgs == 0) {
        return dumpsys(fd);
    }

    string option = string(args[0]);
    if (EqualsIgnoreCase(option, "--help")) {
        return cmdHelp(fd);
    } else if (EqualsIgnoreCase(option, "--request")) {
        return cmdRequestFocus(fd, args, numArgs);
    } else if (EqualsIgnoreCase(option, "--abandon")) {
        return cmdAbandonFocus(fd, args, numArgs);
    } else if (EqualsIgnoreCase(option, "--requestFocusWithMetaData")) {
        return cmdRequestFocusWithMetaData(fd, args, numArgs);
    } else if (EqualsIgnoreCase(option, "--abandonFocusWithMetaData")) {
        return cmdAbandonFocusWithMetaData(fd, args, numArgs);
    } else if (EqualsIgnoreCase(option, "--audioGainCallback")) {
        return cmdOnAudioDeviceGainsChanged(fd, args, numArgs);
    } else {
        dprintf(fd, "Invalid option: %s\n", option.c_str());
        return STATUS_BAD_VALUE;
    }
}

binder_status_t AudioControl::dumpsys(int fd) {
    if (mFocusListener == nullptr) {
        dprintf(fd, "No focus listener registered\n");
    } else {
        dprintf(fd, "Focus listener registered\n");
    }
    dprintf(fd, "AudioGainCallback %sregistered\n", (mAudioGainCallback == nullptr ? "NOT " : ""));
    return STATUS_OK;
}

binder_status_t AudioControl::cmdHelp(int fd) const {
    dprintf(fd, "Usage: \n\n");
    dprintf(fd, "[no args]: dumps focus listener / gain callback registered status\n");
    dprintf(fd, "--help: shows this help\n");
    dprintf(fd,
            "--request <USAGE> <ZONE_ID> <FOCUS_GAIN>: requests audio focus for specified "
            "usage (string), audio zone ID (int), and focus gain type (int)\n"
            "Deprecated, use MetaData instead\n");
    dprintf(fd,
            "--abandon <USAGE> <ZONE_ID>: abandons audio focus for specified usage (string) and "
            "audio zone ID (int)\n"
            "Deprecated, use MetaData instead\n");
    dprintf(fd, "See audio_policy_configuration.xsd for valid AudioUsage values.\n");

    dprintf(fd,
            "--requestFocusWithMetaData <METADATA> <ZONE_ID> <FOCUS_GAIN>: "
            "requests audio focus for specified metadata, audio zone ID (int), "
            "and focus gain type (int)\n");
    dprintf(fd,
            "--abandonFocusWithMetaData <METADATA> <ZONE_ID>: "
            "abandons audio focus for specified metadata and audio zone ID (int)\n");
    dprintf(fd,
            "--audioGainCallback <ZONE_ID> <REASON_1>[,<REASON_N> ...]"
            "<DEVICE_ADDRESS_1> <GAIN_INDEX_1> [<DEVICE_ADDRESS_N> <GAIN_INDEX_N> ...]: fire audio "
            "gain callback for audio zone ID (int), the given reasons (csv int) for given pairs "
            "of device address (string) and gain index (int) \n");

    dprintf(fd,
            "Note on <METADATA>: <USAGE,CONTENT_TYPE[,TAGS]>  specified as where (int)usage, "
            "(int)content type and tags (string)string)\n");
    dprintf(fd,
            "See android/media/audio/common/AudioUsageType.aidl for valid AudioUsage values.\n");
    dprintf(fd,
            "See android/media/audio/common/AudioContentType.aidl for valid AudioContentType "
            "values.\n");
    dprintf(fd,
            "Tags are optional. If provided, it must follow the <key>=<value> pattern, where the "
            "value is namespaced (for example com.google.strategy=VR).\n");

    return STATUS_OK;
}

binder_status_t AudioControl::cmdRequestFocus(int fd, const char** args, uint32_t numArgs) {
    if (!checkCallerHasWritePermissions(fd)) {
        return STATUS_PERMISSION_DENIED;
    }
    if (numArgs != 4) {
        dprintf(fd,
                "Invalid number of arguments: please provide --request <USAGE> <ZONE_ID> "
                "<FOCUS_GAIN>\n");
        return STATUS_BAD_VALUE;
    }

    string usage = string(args[1]);
    if (xsd::isUnknownAudioUsage(usage)) {
        dprintf(fd,
                "Unknown usage provided: %s. Please see audio_policy_configuration.xsd V7_0 "
                "for supported values\n",
                usage.c_str());
        return STATUS_BAD_VALUE;
    }

    int zoneId;
    if (!safelyParseInt(string(args[2]), &zoneId)) {
        dprintf(fd, "Non-integer zoneId provided with request: %s\n", string(args[2]).c_str());
        return STATUS_BAD_VALUE;
    }

    int focusGainValue;
    if (!safelyParseInt(string(args[3]), &focusGainValue)) {
        dprintf(fd, "Non-integer focusGain provided with request: %s\n", string(args[3]).c_str());
        return STATUS_BAD_VALUE;
    }
    AudioFocusChange focusGain = AudioFocusChange(focusGainValue);

    if (mFocusListener == nullptr) {
        dprintf(fd, "Unable to request focus - no focus listener registered\n");
        return STATUS_BAD_VALUE;
    }

    mFocusListener->requestAudioFocus(usage, zoneId, focusGain);
    dprintf(fd, "Requested focus for usage %s, zoneId %d, and focusGain %d\n", usage.c_str(),
            zoneId, focusGain);
    return STATUS_OK;
}

binder_status_t AudioControl::cmdAbandonFocus(int fd, const char** args, uint32_t numArgs) {
    if (!checkCallerHasWritePermissions(fd)) {
        return STATUS_PERMISSION_DENIED;
    }
    if (numArgs != 3) {
        dprintf(fd, "Invalid number of arguments: please provide --abandon <USAGE> <ZONE_ID>\n");
        return STATUS_BAD_VALUE;
    }

    string usage = string(args[1]);
    if (xsd::isUnknownAudioUsage(usage)) {
        dprintf(fd,
                "Unknown usage provided: %s. Please see audio_policy_configuration.xsd V7_0 "
                "for supported values\n",
                usage.c_str());
        return STATUS_BAD_VALUE;
    }

    int zoneId;
    if (!safelyParseInt(string(args[2]), &zoneId)) {
        dprintf(fd, "Non-integer zoneId provided with abandon: %s\n", string(args[2]).c_str());
        return STATUS_BAD_VALUE;
    }

    if (mFocusListener == nullptr) {
        dprintf(fd, "Unable to abandon focus - no focus listener registered\n");
        return STATUS_BAD_VALUE;
    }

    mFocusListener->abandonAudioFocus(usage, zoneId);
    dprintf(fd, "Abandoned focus for usage %s and zoneId %d\n", usage.c_str(), zoneId);
    return STATUS_OK;
}

binder_status_t AudioControl::parseMetaData(int fd, const std::string& metadataLiteral,
                                            audiohalcommon::PlaybackTrackMetadata& trackMetadata) {
    std::stringstream csvMetaData(metadataLiteral);
    std::vector<std::string> splitMetaData;
    std::string attribute;
    while (getline(csvMetaData, attribute, ',')) {
        splitMetaData.push_back(attribute);
    }
    if (splitMetaData.size() != 2 && splitMetaData.size() != 3) {
        dprintf(fd,
                "Invalid metadata: %s, please provide <METADATA> as <USAGE,CONTENT_TYPE[,TAGS]> "
                "where (int)usage, (int)content type and tags (string)string)\n",
                metadataLiteral.c_str());
        return STATUS_BAD_VALUE;
    }
    int usage;
    if (!safelyParseInt(splitMetaData[0], &usage)) {
        dprintf(fd, "Non-integer usage provided with request: %s\n", splitMetaData[0].c_str());
        return STATUS_BAD_VALUE;
    }
    int contentType;
    if (!safelyParseInt(splitMetaData[1], &contentType)) {
        dprintf(fd, "Non-integer content type provided with request: %s\n",
                splitMetaData[1].c_str());
        return STATUS_BAD_VALUE;
    }
    const std::string tags = (splitMetaData.size() == 3 ? splitMetaData[2] : "");

    trackMetadata = {.usage = static_cast<audiomediacommon::AudioUsage>(usage),
                     .contentType = static_cast<audiomediacommon::AudioContentType>(contentType),
                     .tags = {tags}};
    return STATUS_OK;
}

binder_status_t AudioControl::cmdRequestFocusWithMetaData(int fd, const char** args,
                                                          uint32_t numArgs) {
    if (!checkCallerHasWritePermissions(fd)) {
        return STATUS_PERMISSION_DENIED;
    }
    if (numArgs != 4) {
        dprintf(fd,
                "Invalid number of arguments: please provide:\n"
                "--requestFocusWithMetaData <METADATA> <ZONE_ID> <FOCUS_GAIN>: "
                "requests audio focus for specified metadata, audio zone ID (int), "
                "and focus gain type (int)\n");
        return STATUS_BAD_VALUE;
    }
    std::string metadataLiteral = std::string(args[1]);
    audiohalcommon::PlaybackTrackMetadata trackMetadata{};
    auto status = parseMetaData(fd, metadataLiteral, trackMetadata);
    if (status != STATUS_OK) {
        return status;
    }

    int zoneId;
    if (!safelyParseInt(std::string(args[2]), &zoneId)) {
        dprintf(fd, "Non-integer zoneId provided with request: %s\n", std::string(args[2]).c_str());
        return STATUS_BAD_VALUE;
    }

    int focusGainValue;
    if (!safelyParseInt(std::string(args[3]), &focusGainValue)) {
        dprintf(fd, "Non-integer focusGain provided with request: %s\n",
                std::string(args[3]).c_str());
        return STATUS_BAD_VALUE;
    }
    AudioFocusChange focusGain = AudioFocusChange(focusGainValue);

    if (mFocusListener == nullptr) {
        dprintf(fd, "Unable to request focus - no focus listener registered\n");
        return STATUS_BAD_VALUE;
    }
    mFocusListener->requestAudioFocusWithMetaData(trackMetadata, zoneId, focusGain);
    dprintf(fd, "Requested focus for metadata %s, zoneId %d, and focusGain %d\n",
            trackMetadata.toString().c_str(), zoneId, focusGain);
    return STATUS_OK;
}

binder_status_t AudioControl::cmdAbandonFocusWithMetaData(int fd, const char** args,
                                                          uint32_t numArgs) {
    if (!checkCallerHasWritePermissions(fd)) {
        return STATUS_PERMISSION_DENIED;
    }
    if (numArgs != 3) {
        dprintf(fd,
                "Invalid number of arguments: please provide:\n"
                "--abandonFocusWithMetaData <METADATA> <ZONE_ID>: "
                "abandons audio focus for specified metadata and audio zone ID (int)\n");
        return STATUS_BAD_VALUE;
    }
    std::string metadataLiteral = std::string(args[1]);
    audiohalcommon::PlaybackTrackMetadata trackMetadata{};
    auto status = parseMetaData(fd, metadataLiteral, trackMetadata);
    if (status != STATUS_OK) {
        return status;
    }
    int zoneId;
    if (!safelyParseInt(std::string(args[2]), &zoneId)) {
        dprintf(fd, "Non-integer zoneId provided with request: %s\n", std::string(args[2]).c_str());
        return STATUS_BAD_VALUE;
    }
    if (mFocusListener == nullptr) {
        dprintf(fd, "Unable to abandon focus - no focus listener registered\n");
        return STATUS_BAD_VALUE;
    }

    mFocusListener->abandonAudioFocusWithMetaData(trackMetadata, zoneId);
    dprintf(fd, "Abandoned focus for metadata %s and zoneId %d\n", trackMetadata.toString().c_str(),
            zoneId);
    return STATUS_OK;
}

binder_status_t AudioControl::cmdOnAudioDeviceGainsChanged(int fd, const char** args,
                                                           uint32_t numArgs) {
    if (!checkCallerHasWritePermissions(fd)) {
        return STATUS_PERMISSION_DENIED;
    }
    if ((numArgs + 1) % 2 != 0) {
        dprintf(fd,
                "Invalid number of arguments: please provide\n"
                "--audioGainCallback <ZONE_ID> <REASON_1>[,<REASON_N> ...]"
                "<DEVICE_ADDRESS_1> <GAIN_INDEX_1> [<DEVICE_ADDRESS_N> <GAIN_INDEX_N> ...]: "
                "fire audio gain callback for audio zone ID (int), "
                "with the given reasons (csv int) for given pairs of device address (string) "
                "and gain index (int) \n");
        return STATUS_BAD_VALUE;
    }
    int zoneId;
    if (!safelyParseInt(string(args[1]), &zoneId)) {
        dprintf(fd, "Non-integer zoneId provided with request: %s\n", std::string(args[1]).c_str());
        return STATUS_BAD_VALUE;
    }
    std::string reasonsLiteral = std::string(args[2]);
    std::stringstream csvReasonsLiteral(reasonsLiteral);
    std::vector<Reasons> reasons;
    std::string reasonLiteral;
    while (getline(csvReasonsLiteral, reasonLiteral, ',')) {
        int reason;
        if (!safelyParseInt(reasonLiteral, &reason)) {
            dprintf(fd, "Invalid Reason(s) provided %s\n", reasonLiteral.c_str());
            return STATUS_BAD_VALUE;
        }
        reasons.push_back(static_cast<Reasons>(reason));
    }

    std::vector<AudioGainConfigInfo> agcis{};
    for (uint32_t i = 3; i < numArgs; i += 2) {
        std::string deviceAddress = std::string(args[i]);
        int32_t index;
        if (!safelyParseInt(std::string(args[i + 1]), &index)) {
            dprintf(fd, "Non-integer index provided with request: %s\n",
                    std::string(args[i + 1]).c_str());
            return STATUS_BAD_VALUE;
        }
        AudioGainConfigInfo agci{zoneId, deviceAddress, index};
        agcis.push_back(agci);
    }
    if (mAudioGainCallback == nullptr) {
        dprintf(fd,
                "Unable to trig audio gain callback for reasons=%s and gains=%s\n"
                " - no audio gain callback registered\n",
                toEnumString(reasons).c_str(), toString(agcis).c_str());
        return STATUS_BAD_VALUE;
    }

    mAudioGainCallback->onAudioDeviceGainsChanged(reasons, agcis);
    dprintf(fd, "Fired audio gain callback for reasons=%s and gains=%s\n",
            toEnumString(reasons).c_str(), toString(agcis).c_str());
    return STATUS_OK;
}
}  // namespace aidl::android::hardware::automotive::audiocontrol
