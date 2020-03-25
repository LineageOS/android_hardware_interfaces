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

#include <android-base/properties.h>
#include <android/hardware/dumpstate/1.1/IDumpstateDevice.h>
#include <android/hardware/dumpstate/1.1/types.h>
#include <hidl/HidlLazyUtils.h>
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include <log/log.h>

#include "DumpstateUtil.h"

namespace {
using ::android::hardware::hidl_handle;
using ::android::hardware::Return;
using ::android::hardware::Void;

using ::android::hardware::dumpstate::V1_1::DumpstateMode;
using ::android::hardware::dumpstate::V1_1::DumpstateStatus;
using ::android::hardware::dumpstate::V1_1::IDumpstateDevice;

using ::android::os::dumpstate::DumpFileToFd;

const char kVerboseLoggingProperty[] = "persist.dumpstate.verbose_logging.enabled";

struct DumpstateDevice : public IDumpstateDevice {
    // 1.1
    Return<DumpstateStatus> dumpstateBoard_1_1(const hidl_handle& handle, const DumpstateMode mode,
                                               uint64_t /*timeoutMillis*/) override {
        if (handle == nullptr || handle->numFds < 1) {
            ALOGE("no FDs\n");
            return DumpstateStatus::ILLEGAL_ARGUMENT;
        }

        int fd = handle->data[0];
        if (fd < 0) {
            ALOGE("invalid FD: %d\n", fd);
            return DumpstateStatus::ILLEGAL_ARGUMENT;
        }

        switch (mode) {
            case DumpstateMode::FULL:
                return dumpstateBoardImpl(fd, true);

            case DumpstateMode::DEFAULT:
                return dumpstateBoardImpl(fd, false);

            case DumpstateMode::INTERACTIVE:
            case DumpstateMode::REMOTE:
            case DumpstateMode::WEAR:
            case DumpstateMode::CONNECTIVITY:
            case DumpstateMode::WIFI:
            case DumpstateMode::PROTO:
                ALOGE("The requested mode is not supported: %s\n", toString(mode).c_str());
                return DumpstateStatus::UNSUPPORTED_MODE;

            default:
                ALOGE("The requested mode is invalid: %s\n", toString(mode).c_str());
                return DumpstateStatus::ILLEGAL_ARGUMENT;
        }
    }

    Return<void> setVerboseLoggingEnabled(bool enable) override {
        ::android::base::SetProperty(kVerboseLoggingProperty, enable ? "true" : "false");
        return Void();
    }

    Return<bool> getVerboseLoggingEnabled() override { return getVerboseLoggingEnabledImpl(); }

    // 1.0
    Return<void> dumpstateBoard(const hidl_handle& h) override {
        dumpstateBoard_1_1(h, DumpstateMode::DEFAULT, 0);
        return Void();
    }

    DumpstateStatus dumpstateBoardImpl(const int fd, const bool full) {
        ALOGD("DumpstateDevice::dumpstateBoard() FD: %d\n", fd);
        ALOGI("Dumpstate HIDL not provided by device\n");

        dprintf(fd, "verbose logging: %s\n",
                getVerboseLoggingEnabledImpl() ? "enabled" : "disabled");

        dprintf(fd, "[%s] %s\n", (full ? "full" : "default"), "Hello, world!");

        // Shows an example on how to use the libdumpstateutil API.
        DumpFileToFd(fd, "cmdline", "/proc/self/cmdline");

        return DumpstateStatus::OK;
    }

    static bool getVerboseLoggingEnabledImpl() {
        return ::android::base::GetBoolProperty(kVerboseLoggingProperty, false);
    }
};
}  // namespace

int main(int, char**) {
    using ::android::sp;
    using ::android::hardware::configureRpcThreadpool;
    using ::android::hardware::joinRpcThreadpool;
    using ::android::hardware::LazyServiceRegistrar;

    configureRpcThreadpool(1, true);

    sp<DumpstateDevice> dumpstate(new DumpstateDevice);
    auto serviceRegistrar = LazyServiceRegistrar::getInstance();

    if (serviceRegistrar.registerService(dumpstate) != ::android::OK) {
        ALOGE("Could not register service.");
        return 1;
    }

    joinRpcThreadpool();
    return 0;
}
