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
    return STATUS_OK;
}

binder_status_t AudioControl::cmdHelp(int fd) const {
    dprintf(fd, "Usage: \n\n");
    dprintf(fd, "[no args]: dumps focus listener status\n");
    dprintf(fd, "--help: shows this help\n");
    dprintf(fd,
            "--request <USAGE> <ZONE_ID> <FOCUS_GAIN>: requests audio focus for specified "
            "usage (string), audio zone ID (int), and focus gain type (int)\n");
    dprintf(fd,
            "--abandon <USAGE> <ZONE_ID>: abandons audio focus for specified usage (string) and "
            "audio zone ID (int)\n");
    dprintf(fd, "See audio_policy_configuration.xsd for valid AudioUsage values.\n");
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

}  // namespace aidl::android::hardware::automotive::audiocontrol
