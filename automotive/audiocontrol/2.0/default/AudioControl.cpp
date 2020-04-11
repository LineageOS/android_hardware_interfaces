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

#include "AudioControl.h"

#include <stdio.h>

#include <android-base/logging.h>
#include <android-base/parseint.h>
#include <android-base/strings.h>

#include <hidl/HidlTransportSupport.h>
#include <hwbinder/IPCThreadState.h>
#include <private/android_filesystem_config.h>

#include "CloseHandle.h"

namespace android::hardware::automotive::audiocontrol::V2_0::implementation {

using ::android::base::EqualsIgnoreCase;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_string;

static const float kLowerBound = -1.0f;
static const float kUpperBound = 1.0f;

AudioControl::AudioControl() {}

Return<sp<ICloseHandle>> AudioControl::registerFocusListener(const sp<IFocusListener>& listener) {
    LOG(DEBUG) << "registering focus listener";
    sp<ICloseHandle> closeHandle(nullptr);

    if (listener) {
        mFocusListener = listener;

        closeHandle = new CloseHandle([this, listener]() {
            if (mFocusListener == listener) {
                mFocusListener = nullptr;
            }
        });
    } else {
        LOG(ERROR) << "Unexpected nullptr for listener resulting in no-op.";
    }

    return closeHandle;
}

Return<void> AudioControl::setBalanceTowardRight(float value) {
    if (isValidValue(value)) {
        // Just log in this default mock implementation
        LOG(INFO) << "Balance set to " << value;
    } else {
        LOG(ERROR) << "Balance value out of range -1 to 1 at " << value;
    }
    return Void();
}

Return<void> AudioControl::setFadeTowardFront(float value) {
    if (!isValidValue(value)) {
        // Just log in this default mock implementation
        LOG(INFO) << "Fader set to " << value;
    } else {
        LOG(ERROR) << "Fader value out of range -1 to 1 at " << value;
    }
    return Void();
}

bool AudioControl::isValidValue(float value) {
    return (value > kLowerBound) && (value < kUpperBound);
}

Return<void> AudioControl::onAudioFocusChange(hidl_bitfield<AudioUsage> usage, int zoneId,
                                              hidl_bitfield<AudioFocusChange> focusChange) {
    LOG(INFO) << "Focus changed: " << static_cast<int>(focusChange) << " for usage "
              << static_cast<int>(usage) << " in zone " << zoneId;
    return Void();
}

Return<void> AudioControl::debug(const hidl_handle& fd, const hidl_vec<hidl_string>& options) {
    if (fd.getNativeHandle() == nullptr || fd->numFds == 0) {
        LOG(ERROR) << "Invalid parameters passed to debug()";
        return Void();
    }

    cmdDump(fd->data[0], options);
    return Void();
}

void AudioControl::cmdDump(int fd, const hidl_vec<hidl_string>& options) {
    if (options.size() == 0) {
        dump(fd);
        return;
    }

    std::string option = options[0];
    if (EqualsIgnoreCase(option, "--help")) {
        cmdHelp(fd);
    } else if (EqualsIgnoreCase(option, "--request")) {
        cmdRequestFocus(fd, options);
    } else if (EqualsIgnoreCase(option, "--abandon")) {
        cmdAbandonFocus(fd, options);
    } else {
        dprintf(fd, "Invalid option: %s\n", option.c_str());
    }
}

void AudioControl::dump(int fd) {
    if (mFocusListener == nullptr) {
        dprintf(fd, "No focus listener registered\n");
    } else {
        dprintf(fd, "Focus listener registered\n");
    }
}

void AudioControl::cmdHelp(int fd) const {
    dprintf(fd, "Usage: \n\n");
    dprintf(fd, "[no args]: dumps focus listener status\n");
    dprintf(fd, "--help: shows this help\n");
    dprintf(fd,
            "--request <USAGE> <ZONE_ID> <FOCUS_GAIN>: requests audio focus for specified "
            "usage (int), audio zone ID (int), and focus gain type (int)\n");
    dprintf(fd,
            "--abandon <USAGE> <ZONE_ID>: abandons audio focus for specified usage (int) and "
            "audio zone ID (int)\n");
}

void AudioControl::cmdRequestFocus(int fd, const hidl_vec<hidl_string>& options) {
    if (!checkCallerHasWritePermissions(fd) || !checkArgumentsSize(fd, options, 3)) return;

    hidl_bitfield<AudioUsage> usage;
    if (!safelyParseInt(options[1], &usage)) {
        dprintf(fd, "Non-integer usage provided with request: %s\n", options[1].c_str());
        return;
    }
    int zoneId;
    if (!safelyParseInt(options[2], &zoneId)) {
        dprintf(fd, "Non-integer zoneId provided with request: %s\n", options[2].c_str());
        return;
    }
    hidl_bitfield<AudioFocusChange> focusGain;
    if (!safelyParseInt(options[3], &focusGain)) {
        dprintf(fd, "Non-integer focusGain provided with request: %s\n", options[3].c_str());
        return;
    }

    if (mFocusListener == nullptr) {
        dprintf(fd, "Unable to request focus - no focus listener registered\n");
        return;
    }

    mFocusListener->requestAudioFocus(usage, zoneId, focusGain);
    dprintf(fd, "Requested focus for usage %d, zoneId %d, and focusGain %d\n", usage, zoneId,
            focusGain);
}

void AudioControl::cmdAbandonFocus(int fd, const hidl_vec<hidl_string>& options) {
    if (!checkCallerHasWritePermissions(fd) || !checkArgumentsSize(fd, options, 2)) return;

    hidl_bitfield<AudioUsage> usage;
    if (!safelyParseInt(options[1], &usage)) {
        dprintf(fd, "Non-integer usage provided with abandon: %s\n", options[1].c_str());
        return;
    }
    int zoneId;
    if (!safelyParseInt(options[2], &zoneId)) {
        dprintf(fd, "Non-integer zoneId provided with abandon: %s\n", options[2].c_str());
        return;
    }

    if (mFocusListener == nullptr) {
        dprintf(fd, "Unable to abandon focus - no focus listener registered\n");
        return;
    }

    mFocusListener->abandonAudioFocus(usage, zoneId);
    dprintf(fd, "Abandoned focus for usage %d and zoneId %d\n", usage, zoneId);
}

bool AudioControl::checkCallerHasWritePermissions(int fd) {
    // Double check that's only called by root - it should be be blocked at the HIDL debug() level,
    // but it doesn't hurt to make sure...
    if (hardware::IPCThreadState::self()->getCallingUid() != AID_ROOT) {
        dprintf(fd, "Must be root\n");
        return false;
    }
    return true;
}

bool AudioControl::checkArgumentsSize(int fd, const hidl_vec<hidl_string>& options,
                                      size_t expectedSize) {
    // options includes the command, so reducing size by one
    size_t size = options.size() - 1;
    if (size == expectedSize) {
        return true;
    }
    dprintf(fd, "Invalid number of arguments: required %zu, got %zu\n", expectedSize, size);
    return false;
}

bool AudioControl::safelyParseInt(std::string s, int* out) {
    if (!android::base::ParseInt(s, out)) {
        return false;
    }
    return true;
}

}  // namespace android::hardware::automotive::audiocontrol::V2_0::implementation
