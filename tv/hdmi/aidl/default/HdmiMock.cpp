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

#define LOG_TAG "android.hardware.tv.hdmi"
#include <android-base/logging.h>
#include <fcntl.h>
#include <utils/Log.h>

#include "HdmiMock.h"

using ndk::ScopedAStatus;

namespace android {
namespace hardware {
namespace tv {
namespace hdmi {
namespace implementation {

void HdmiMock::serviceDied(void* cookie) {
    ALOGE("HdmiMock died");
    auto hdmi = static_cast<HdmiMock*>(cookie);
    hdmi->mHdmiThreadRun = false;
}

ScopedAStatus HdmiMock::getPortInfo(std::vector<HdmiPortInfo>* _aidl_return) {
    *_aidl_return = mPortInfos;
    return ScopedAStatus::ok();
}

ScopedAStatus HdmiMock::isConnected(int32_t portId, bool* _aidl_return) {
    // Maintain port connection status and update on hotplug event
    if (portId <= mTotalPorts && portId >= 1) {
        *_aidl_return = mPortConnectionStatus[portId];
    } else {
        *_aidl_return = false;
    }

    return ScopedAStatus::ok();
}

ScopedAStatus HdmiMock::setCallback(const std::shared_ptr<IHdmiCallback>& callback) {
    if (mCallback != nullptr) {
        mCallback = nullptr;
    }

    if (callback != nullptr) {
        mCallback = callback;
        AIBinder_linkToDeath(this->asBinder().get(), mDeathRecipient.get(), 0 /* cookie */);

        mInputFile = open(HDMI_MSG_IN_FIFO, O_RDWR | O_CLOEXEC);
        pthread_create(&mThreadId, NULL, __threadLoop, this);
        pthread_setname_np(mThreadId, "hdmi_loop");
    }
    return ScopedAStatus::ok();
}

ScopedAStatus HdmiMock::setHpdSignal(HpdSignal signal) {
    if (mHdmiThreadRun) {
        mHpdSignal = signal;
        return ScopedAStatus::ok();
    } else {
        return ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::FAILURE_INVALID_STATE));
    }
}

ScopedAStatus HdmiMock::getHpdSignal(HpdSignal* _aidl_return) {
    *_aidl_return = mHpdSignal;
    return ScopedAStatus::ok();
}

void* HdmiMock::__threadLoop(void* user) {
    HdmiMock* const self = static_cast<HdmiMock*>(user);
    self->threadLoop();
    return 0;
}

int HdmiMock::readMessageFromFifo(unsigned char* buf, int msgCount) {
    if (msgCount <= 0 || !buf) {
        return 0;
    }

    int ret = -1;
    // Maybe blocked at driver
    ret = read(mInputFile, buf, msgCount);
    if (ret < 0) {
        ALOGE("[halimp_aidl] read :%s failed, ret:%d\n", HDMI_MSG_IN_FIFO, ret);
        return -1;
    }

    return ret;
}

void HdmiMock::printEventBuf(const char* msg_buf, int len) {
    int i, size = 0;
    const int bufSize = MESSAGE_BODY_MAX_LENGTH * 3;
    // Use 2 characters for each byte in the message plus 1 space
    char buf[bufSize] = {0};

    // Messages longer than max length will be truncated.
    for (i = 0; i < len && size < bufSize; i++) {
        size += sprintf(buf + size, " %02x", msg_buf[i]);
    }
    ALOGD("[halimp_aidl] %s, msg:%.*s", __FUNCTION__, size, buf);
}

void HdmiMock::handleHotplugMessage(unsigned char* msgBuf) {
    bool connected = ((msgBuf[3]) & 0xf) > 0;
    int32_t portId = static_cast<uint32_t>(msgBuf[0] & 0xf);

    if (portId > static_cast<int32_t>(mPortInfos.size())) {
        ALOGD("[halimp_aidl] ignore hot plug message, id %x does not exist", portId);
        return;
    }

    ALOGD("[halimp_aidl] hot plug port id %x, is connected %x", (msgBuf[0] & 0xf),
          (msgBuf[3] & 0xf));
    mPortConnectionStatus[portId] = connected;
    if (mPortInfos[portId].type == HdmiPortType::OUTPUT) {
        mPhysicalAddress = (connected ? 0xffff : ((msgBuf[1] << 8) | (msgBuf[2])));
        mPortInfos[portId].physicalAddress = mPhysicalAddress;
        ALOGD("[halimp_aidl] hot plug physical address %x", mPhysicalAddress);
    }

    if (mCallback != nullptr) {
        mCallback->onHotplugEvent(connected, portId);
    }
}

void HdmiMock::threadLoop() {
    ALOGD("[halimp_aidl] threadLoop start.");
    unsigned char msgBuf[MESSAGE_BODY_MAX_LENGTH];
    int r = -1;

    // Open the input pipe
    while (mInputFile < 0) {
        usleep(1000 * 1000);
        mInputFile = open(HDMI_MSG_IN_FIFO, O_RDONLY | O_CLOEXEC);
    }
    ALOGD("[halimp_aidl] file open ok, fd = %d.", mInputFile);

    while (mHdmiThreadRun) {
        memset(msgBuf, 0, sizeof(msgBuf));
        // Try to get a message from dev.
        // echo -n -e '\x04\x83' >> /dev/cec
        r = readMessageFromFifo(msgBuf, MESSAGE_BODY_MAX_LENGTH);
        if (r <= 1) {
            // Ignore received ping messages
            continue;
        }

        printEventBuf((const char*)msgBuf, r);

        if (((msgBuf[0] >> 4) & 0xf) == 0xf) {
            handleHotplugMessage(msgBuf);
        }
    }

    ALOGD("[halimp_aidl] thread end.");
}

HdmiMock::HdmiMock() {
    ALOGE("[halimp_aidl] Opening a virtual HDMI HAL for testing and virtual machine.");
    mCallback = nullptr;
    mPortInfos.resize(mTotalPorts);
    mPortConnectionStatus.resize(mTotalPorts);
    mPortInfos[0] = {.type = HdmiPortType::OUTPUT,
                     .portId = static_cast<uint32_t>(1),
                     .cecSupported = true,
                     .arcSupported = false,
                     .eArcSupported = false,
                     .physicalAddress = mPhysicalAddress};
    mPortConnectionStatus[0] = false;
    mDeathRecipient = ndk::ScopedAIBinder_DeathRecipient(AIBinder_DeathRecipient_new(serviceDied));
}

}  // namespace implementation
}  // namespace hdmi
}  // namespace tv
}  // namespace hardware
}  // namespace android
