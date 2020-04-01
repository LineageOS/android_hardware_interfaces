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

#include <android-base/logging.h>
#include <android-base/strings.h>
#include <hidl/HidlTransportSupport.h>

#include <stdio.h>

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
}

}  // namespace android::hardware::automotive::audiocontrol::V2_0::implementation
