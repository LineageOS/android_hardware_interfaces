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
#include <android-base/parsebool.h>
#include <android-base/parseint.h>
#include <android-base/strings.h>

#include <android_audio_policy_configuration_V7_0-enums.h>
#include <private/android_filesystem_config.h>

#include <numeric>

#include <stdio.h>

namespace aidl::android::hardware::automotive::audiocontrol {

using ::android::base::EqualsIgnoreCase;
using ::android::base::ParseBool;
using ::android::base::ParseBoolResult;
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

namespace {
using ::aidl::android::media::audio::common::AudioChannelLayout;
using ::aidl::android::media::audio::common::AudioDeviceDescription;
using ::aidl::android::media::audio::common::AudioDeviceType;
using ::aidl::android::media::audio::common::AudioFormatDescription;
using ::aidl::android::media::audio::common::AudioFormatType;
using ::aidl::android::media::audio::common::AudioGain;
using ::aidl::android::media::audio::common::AudioGainMode;
using ::aidl::android::media::audio::common::AudioIoFlags;
using ::aidl::android::media::audio::common::AudioOutputFlags;
using ::aidl::android::media::audio::common::AudioPort;
using ::aidl::android::media::audio::common::AudioPortDeviceExt;
using ::aidl::android::media::audio::common::AudioPortExt;
using ::aidl::android::media::audio::common::AudioPortMixExt;
using ::aidl::android::media::audio::common::AudioProfile;
using ::aidl::android::media::audio::common::PcmType;

// reuse common code artifacts
void fillProfile(const std::vector<int32_t>& channelLayouts,
                 const std::vector<int32_t>& sampleRates, AudioProfile* profile) {
    for (auto layout : channelLayouts) {
        profile->channelMasks.push_back(
                AudioChannelLayout::make<AudioChannelLayout::layoutMask>(layout));
    }
    profile->sampleRates.insert(profile->sampleRates.end(), sampleRates.begin(), sampleRates.end());
}

AudioProfile createProfile(PcmType pcmType, const std::vector<int32_t>& channelLayouts,
                           const std::vector<int32_t>& sampleRates) {
    AudioProfile profile;
    profile.format.type = AudioFormatType::PCM;
    profile.format.pcm = pcmType;
    fillProfile(channelLayouts, sampleRates, &profile);
    return profile;
}

AudioProfile createProfile(const std::string& encodingType,
                           const std::vector<int32_t>& channelLayouts,
                           const std::vector<int32_t>& sampleRates) {
    AudioProfile profile;
    profile.format.encoding = encodingType;
    fillProfile(channelLayouts, sampleRates, &profile);
    return profile;
}

AudioPortExt createDeviceExt(AudioDeviceType devType, int32_t flags,
                             const std::string& connection = "", const std::string& address = "") {
    AudioPortDeviceExt deviceExt;
    deviceExt.device.type.type = devType;
    if (devType == AudioDeviceType::IN_MICROPHONE && connection.empty()) {
        deviceExt.device.address = "bottom";
    } else if (devType == AudioDeviceType::IN_MICROPHONE_BACK && connection.empty()) {
        deviceExt.device.address = "back";
    } else {
        deviceExt.device.address = std::move(address);
    }
    deviceExt.device.type.connection = std::move(connection);
    deviceExt.flags = flags;
    return AudioPortExt::make<AudioPortExt::Tag::device>(deviceExt);
}

AudioPort createPort(int32_t id, const std::string& name, int32_t flags, bool isInput,
                     const AudioPortExt& ext) {
    AudioPort port;
    port.id = id;
    port.name = name;
    port.flags = isInput ? AudioIoFlags::make<AudioIoFlags::Tag::input>(flags)
                         : AudioIoFlags::make<AudioIoFlags::Tag::output>(flags);
    port.ext = ext;
    return port;
}

AudioGain createGain(int32_t mode, AudioChannelLayout channelMask, int32_t minValue,
                     int32_t maxValue, int32_t defaultValue, int32_t stepValue,
                     int32_t minRampMs = 100, int32_t maxRampMs = 100, bool useForVolume = true) {
    AudioGain gain;
    gain.mode = mode;
    gain.channelMask = channelMask;
    gain.minValue = minValue;
    gain.maxValue = maxValue;
    gain.defaultValue = defaultValue;
    gain.stepValue = stepValue;
    gain.minRampMs = minRampMs;
    gain.maxRampMs = maxRampMs;
    gain.useForVolume = useForVolume;
    return gain;
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

ndk::ScopedAStatus AudioControl::setModuleChangeCallback(
        const std::shared_ptr<IModuleChangeCallback>& in_callback) {
    LOG(DEBUG) << ": " << __func__;

    if (in_callback.get() == nullptr) {
        LOG(ERROR) << __func__ << ": Callback is nullptr";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    if (mModuleChangeCallback != nullptr) {
        LOG(ERROR) << __func__ << ": Module change callback was already registered";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    std::atomic_store(&mModuleChangeCallback, in_callback);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus AudioControl::clearModuleChangeCallback() {
    if (mModuleChangeCallback != nullptr) {
        shared_ptr<IModuleChangeCallback> nullCallback = nullptr;
        std::atomic_store(&mModuleChangeCallback, nullCallback);
        LOG(DEBUG) << ":" << __func__ << ": Unregistered successfully";
    } else {
        LOG(DEBUG) << ":" << __func__ << ": No callback registered, no-op";
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
    } else if (EqualsIgnoreCase(option, "--audioPortsChangedCallback")) {
        return cmdOnAudioPortsChanged(fd, args, numArgs);
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
    dprintf(fd,
            "--audioPortsChangedCallback <ID_1> <NAME_1> <BUS_ADDRESS_1> <CONNECTION_TYPE_1> "
            "<AUDIO_GAINS_1> [<ID_N> <NAME_N> <BUS_ADDRESS_N> <CONNECTION_TYPE_N> "
            "<AUDIO_GAINS_N>]: fires audio ports changed callback. Carries list of modified "
            "AudioPorts. "
            "For simplicity, this command accepts limited information for each AudioPort: "
            "id(int), name(string), port address(string), connection type (string), "
            "audio gain (csv int)\n");
    dprintf(fd, "Notes: \n");
    dprintf(fd,
            "1. AudioGain csv should match definition at "
            "android/media/audio/common/AudioPort.aidl\n");
    dprintf(fd,
            "2. See android/media/audio/common/AudioDeviceDescription.aidl for valid "
            "<CONNECTION_TYPE> values.\n");
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

binder_status_t AudioControl::parseAudioGains(int fd, const std::string& stringGain,
                                              std::vector<AudioGain>& gains) {
    const int kAudioGainSize = 9;
    std::stringstream csvGain(stringGain);
    std::vector<std::string> vecGain;
    std::string value;
    while (getline(csvGain, value, ',')) {
        vecGain.push_back(value);
    }

    if ((vecGain.size() == 0) || ((vecGain.size() % kAudioGainSize) != 0)) {
        dprintf(fd, "Erroneous input to generate AudioGain: %s\n", stringGain.c_str());
        return STATUS_BAD_VALUE;
    }

    // iterate over injected AudioGains
    for (int index = 0; index < vecGain.size(); index += kAudioGainSize) {
        int32_t mode;
        if (!safelyParseInt(vecGain[index], &mode)) {
            dprintf(fd, "Non-integer index provided with request: %s\n", vecGain[index].c_str());
            return STATUS_BAD_VALUE;
        }

        // car audio framework only supports JOINT mode.
        // skip injected AudioGains that are not compliant with this.
        if (mode != static_cast<int>(AudioGainMode::JOINT)) {
            LOG(WARNING) << ":" << __func__ << ": skipping gain since it is not JOINT mode!";
            continue;
        }

        int32_t layout;
        if (!safelyParseInt(vecGain[index + 1], &layout)) {
            dprintf(fd, "Non-integer index provided with request: %s\n",
                    vecGain[index + 1].c_str());
            return STATUS_BAD_VALUE;
        }
        AudioChannelLayout channelMask =
                AudioChannelLayout::make<AudioChannelLayout::layoutMask>(layout);

        int32_t minValue;
        if (!safelyParseInt(vecGain[index + 2], &minValue)) {
            dprintf(fd, "Non-integer index provided with request: %s\n",
                    vecGain[index + 2].c_str());
            return STATUS_BAD_VALUE;
        }

        int32_t maxValue;
        if (!safelyParseInt(vecGain[index + 3], &maxValue)) {
            dprintf(fd, "Non-integer index provided with request: %s\n",
                    vecGain[index + 3].c_str());
            return STATUS_BAD_VALUE;
        }

        int32_t defaultValue;
        if (!safelyParseInt(vecGain[index + 4], &defaultValue)) {
            dprintf(fd, "Non-integer index provided with request: %s\n",
                    vecGain[index + 4].c_str());
            return STATUS_BAD_VALUE;
        }

        int32_t stepValue;
        if (!safelyParseInt(vecGain[index + 5], &stepValue)) {
            dprintf(fd, "Non-integer index provided with request: %s\n",
                    vecGain[index + 5].c_str());
            return STATUS_BAD_VALUE;
        }

        int32_t minRampMs;
        if (!safelyParseInt(vecGain[index + 6], &minRampMs)) {
            dprintf(fd, "Non-integer index provided with request: %s\n",
                    vecGain[index + 6].c_str());
            return STATUS_BAD_VALUE;
        }

        int32_t maxRampMs;
        if (!safelyParseInt(vecGain[index + 7], &maxRampMs)) {
            dprintf(fd, "Non-integer index provided with request: %s\n",
                    vecGain[index + 7].c_str());
            return STATUS_BAD_VALUE;
        }

        ParseBoolResult useForVolume = ParseBool(vecGain[index + 8]);
        if (useForVolume == ParseBoolResult::kError) {
            dprintf(fd, "Non-boolean index provided with request: %s\n",
                    vecGain[index + 8].c_str());
            return STATUS_BAD_VALUE;
        } else if (useForVolume == ParseBoolResult::kFalse) {
            // at this level we only care about gain stages that are relevant
            // for volume control. skip the gain stage if its flagged as false.
            LOG(WARNING) << ":" << __func__
                         << ": skipping gain since it is not for volume control!";
            continue;
        }

        AudioGain gain = createGain(mode, channelMask, minValue, maxValue, defaultValue, stepValue,
                                    minRampMs, maxRampMs, true /*useForVolume*/);
        gains.push_back(gain);
    }
    return STATUS_OK;
}

binder_status_t AudioControl::cmdOnAudioPortsChanged(int fd, const char** args, uint32_t numArgs) {
    if (!checkCallerHasWritePermissions(fd)) {
        return STATUS_PERMISSION_DENIED;
    }

    if ((numArgs < 6) || ((numArgs - 1) % 5 != 0)) {
        dprintf(fd,
                "Invalid number of arguments: please provide\n"
                "--audioPortsChangedCallback <ID_1> <NAME_1> <BUS_ADDRESS_1> <CONNECTION_TYPE_1> "
                "<AUDIO_GAINS_1> [<ID_N> <NAME_N> <BUS_ADDRESS_N> <CONNECTION_TYPE_N> "
                "<AUDIO_GAINS_N>]: triggers audio ports changed callback. Carries list of "
                "modified AudioPorts. "
                "For simplicity, this command accepts limited information for each AudioPort: "
                "id(int), name(string), port address(string), connection type (string), "
                "audio gain (csv int)\n");
        return STATUS_BAD_VALUE;
    }

    std::vector<AudioPort> ports;
    for (uint32_t i = 1; i < numArgs; i += 5) {
        binder_status_t status;
        int32_t id;
        if (!safelyParseInt(std::string(args[i]), &id)) {
            dprintf(fd, "Non-integer index provided with request: %s\n",
                    std::string(args[i]).c_str());
            return STATUS_BAD_VALUE;
        }

        std::string name = std::string(args[i + 1]);
        std::string address = std::string(args[i + 2]);
        std::string connection = std::string(args[i + 3]);

        std::string stringGains = std::string(args[i + 4]);
        std::vector<AudioGain> gains;
        status = parseAudioGains(fd, stringGains, gains);
        if (status != STATUS_OK) {
            return status;
        }

        AudioPort port = createPort(
                id, name, 0 /*flags*/, false /*isInput*/,
                createDeviceExt(AudioDeviceType::OUT_DEVICE, 0 /*flags*/, connection, address));
        port.gains.insert(port.gains.begin(), gains.begin(), gains.end());

        ports.push_back(port);
    }

    if (mModuleChangeCallback == nullptr) {
        dprintf(fd,
                "Unable to trigger audio port callback for ports: %s \n"
                " - no module change callback registered\n",
                toString(ports).c_str());
        return STATUS_BAD_VALUE;
    }

    // TODO (b/269139706) fix atomic read issue
    mModuleChangeCallback->onAudioPortsChanged(ports);
    dprintf(fd, "SUCCESS audio port callback for ports: %s \n", toString(ports).c_str());
    return STATUS_OK;
}
}  // namespace aidl::android::hardware::automotive::audiocontrol
