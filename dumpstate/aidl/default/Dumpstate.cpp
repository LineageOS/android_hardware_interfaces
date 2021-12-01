/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include <android-base/properties.h>
#include <log/log.h>
#include "DumpstateUtil.h"

#include "Dumpstate.h"

using android::os::dumpstate::DumpFileToFd;

namespace aidl {
namespace android {
namespace hardware {
namespace dumpstate {

const char kVerboseLoggingProperty[] = "persist.dumpstate.verbose_logging.enabled";

ndk::ScopedAStatus Dumpstate::dumpstateBoard(const std::vector<::ndk::ScopedFileDescriptor>& in_fds,
                                             IDumpstateDevice::DumpstateMode in_mode,
                                             int64_t in_timeoutMillis) {
    (void)in_timeoutMillis;

    if (in_fds.size() < 1) {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                "No file descriptor");
    }

    int fd = in_fds[0].get();
    if (fd < 0) {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                "Invalid file descriptor");
    }

    switch (in_mode) {
        case IDumpstateDevice::DumpstateMode::FULL:
            return dumpstateBoardImpl(fd, true);

        case IDumpstateDevice::DumpstateMode::DEFAULT:
            return dumpstateBoardImpl(fd, false);

        case IDumpstateDevice::DumpstateMode::INTERACTIVE:
        case IDumpstateDevice::DumpstateMode::REMOTE:
        case IDumpstateDevice::DumpstateMode::WEAR:
        case IDumpstateDevice::DumpstateMode::CONNECTIVITY:
        case IDumpstateDevice::DumpstateMode::WIFI:
        case IDumpstateDevice::DumpstateMode::PROTO:
            return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(ERROR_UNSUPPORTED_MODE,
                                                                           "Unsupported mode");

        default:
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "Invalid mode");
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Dumpstate::getVerboseLoggingEnabled(bool* _aidl_return) {
    *_aidl_return = getVerboseLoggingEnabledImpl();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Dumpstate::setVerboseLoggingEnabled(bool in_enable) {
    ::android::base::SetProperty(kVerboseLoggingProperty, in_enable ? "true" : "false");
    return ndk::ScopedAStatus::ok();
}

bool Dumpstate::getVerboseLoggingEnabledImpl() {
    return ::android::base::GetBoolProperty(kVerboseLoggingProperty, false);
}

ndk::ScopedAStatus Dumpstate::dumpstateBoardImpl(const int fd, const bool full) {
    ALOGD("DumpstateDevice::dumpstateBoard() FD: %d\n", fd);

    dprintf(fd, "verbose logging: %s\n", getVerboseLoggingEnabledImpl() ? "enabled" : "disabled");
    dprintf(fd, "[%s] %s\n", (full ? "full" : "default"), "Hello, world!");

    // Shows an example on how to use the libdumpstateutil API.
    DumpFileToFd(fd, "cmdline", "/proc/self/cmdline");

    return ndk::ScopedAStatus::ok();
}

}  // namespace dumpstate
}  // namespace hardware
}  // namespace android
}  // namespace aidl
