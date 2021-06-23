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
#define LOG_TAG "android.hardware.tv.cec@1.0-impl"

#include <android-base/logging.h>
#include <errno.h>
#include <linux/cec.h>
#include <linux/ioctl.h>
#include <sys/eventfd.h>
#include <algorithm>

#include "HdmiCecPort.h"

namespace android {
namespace hardware {
namespace tv {
namespace cec {
namespace V1_0 {
namespace implementation {

HdmiCecPort::HdmiCecPort(unsigned int portId) {
    mPortId = portId;
    mCecFd = -1;
    mExitFd = -1;
}

HdmiCecPort::~HdmiCecPort() {
    release();
}

// Initialise the cec file descriptor
Return<Result> HdmiCecPort::init(const char* path) {
    mCecFd = open(path, O_RDWR);
    if (mCecFd < 0) {
        LOG(ERROR) << "Failed to open " << path << ", Error = " << strerror(errno);
        return Result::FAILURE_NOT_SUPPORTED;
    }
    mExitFd = eventfd(0, EFD_NONBLOCK);
    if (mExitFd < 0) {
        LOG(ERROR) << "Failed to open eventfd, Error = " << strerror(errno);
        release();
        return Result::FAILURE_NOT_SUPPORTED;
    }

    // Ensure the CEC device supports required capabilities
    struct cec_caps caps = {};
    int ret = ioctl(mCecFd, CEC_ADAP_G_CAPS, &caps);
    if (ret) {
        LOG(ERROR) << "Unable to query cec adapter capabilities, Error = " << strerror(errno);
        release();
        return Result::FAILURE_NOT_SUPPORTED;
    }

    if (!(caps.capabilities & (CEC_CAP_LOG_ADDRS | CEC_CAP_TRANSMIT | CEC_CAP_PASSTHROUGH))) {
        LOG(ERROR) << "Wrong cec adapter capabilities " << caps.capabilities;
        release();
        return Result::FAILURE_NOT_SUPPORTED;
    }

    uint32_t mode = CEC_MODE_INITIATOR | CEC_MODE_EXCL_FOLLOWER_PASSTHRU;
    ret = ioctl(mCecFd, CEC_S_MODE, &mode);
    if (ret) {
        LOG(ERROR) << "Unable to set initiator mode, Error = " << strerror(errno);
        release();
        return Result::FAILURE_NOT_SUPPORTED;
    }
    return Result::SUCCESS;
}

Return<void> HdmiCecPort::release() {
    if (mExitFd > 0) {
        uint64_t tmp = 1;
        write(mExitFd, &tmp, sizeof(tmp));
    }
    if (mExitFd > 0) {
        close(mExitFd);
    }
    if (mCecFd > 0) {
        close(mCecFd);
    }
    return Void();
}
}  // namespace implementation
}  // namespace V1_0
}  // namespace cec
}  // namespace tv
}  // namespace hardware
}  // namespace android
