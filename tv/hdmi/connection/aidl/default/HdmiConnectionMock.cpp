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

#define LOG_TAG "android.hardware.tv.hdmi.connection"
#include "HdmiConnectionMock.h"
#include <android-base/logging.h>
#include <fcntl.h>
#include <utils/Log.h>

using ndk::ScopedAStatus;

namespace android {
namespace hardware {
namespace tv {
namespace hdmi {
namespace connection {
namespace implementation {

void HdmiConnectionMock::serviceDied(void* cookie) {
    ALOGE("HdmiConnectionMock died");
    auto hdmi = static_cast<HdmiConnectionMock*>(cookie);
    hdmi->mHdmiThreadRun = false;
    pthread_join(hdmi->mThreadId, NULL);
}

ScopedAStatus HdmiConnectionMock::getPortInfo(std::vector<HdmiPortInfo>* _aidl_return) {
    *_aidl_return = mPortInfos;
    return ScopedAStatus::ok();
}

ScopedAStatus HdmiConnectionMock::isConnected(int32_t portId, bool* _aidl_return) {
    // Maintain port connection status and update on hotplug event
    if (portId <= mTotalPorts && portId >= 1) {
        *_aidl_return = mPortConnectionStatus.at(portId - 1);
    } else {
        *_aidl_return = false;
    }

    return ScopedAStatus::ok();
}

ScopedAStatus HdmiConnectionMock::setCallback(
        const std::shared_ptr<IHdmiConnectionCallback>& callback) {
    if (mCallback != nullptr) {
        stopThread();
        mCallback = nullptr;
    }
    if (callback != nullptr) {
        mCallback = callback;
        mDeathRecipient =
                ndk::ScopedAIBinder_DeathRecipient(AIBinder_DeathRecipient_new(serviceDied));

        AIBinder_linkToDeath(callback->asBinder().get(), mDeathRecipient.get(), this /* cookie */);

        mInputFile = open(HDMI_MSG_IN_FIFO, O_RDWR | O_CLOEXEC);
        pthread_create(&mThreadId, NULL, __threadLoop, this);
        pthread_setname_np(mThreadId, "hdmi_loop");
    }
    return ScopedAStatus::ok();
}

ScopedAStatus HdmiConnectionMock::setHpdSignal(HpdSignal signal, int32_t portId) {
    if (portId > mTotalPorts || portId < 1) {
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    if (!mHdmiThreadRun) {
        return ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::FAILURE_INVALID_STATE));
    }
    mHpdSignal.at(portId - 1) = signal;
    return ScopedAStatus::ok();
}

ScopedAStatus HdmiConnectionMock::getHpdSignal(int32_t portId, HpdSignal* _aidl_return) {
    if (portId > mTotalPorts || portId < 1) {
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    *_aidl_return = mHpdSignal.at(portId - 1);
    return ScopedAStatus::ok();
}

void* HdmiConnectionMock::__threadLoop(void* user) {
    HdmiConnectionMock* const self = static_cast<HdmiConnectionMock*>(user);
    self->threadLoop();
    return 0;
}

int HdmiConnectionMock::readMessageFromFifo(unsigned char* buf, int msgCount) {
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

void HdmiConnectionMock::printEventBuf(const char* msg_buf, int len) {
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

void HdmiConnectionMock::handleHotplugMessage(unsigned char* msgBuf) {
    bool connected = ((msgBuf[3]) & 0xf) > 0;
    int32_t portId = static_cast<uint32_t>(msgBuf[0] & 0xf);

    if (portId > static_cast<int32_t>(mPortInfos.size()) || portId < 1) {
        ALOGD("[halimp_aidl] ignore hot plug message, id %x does not exist", portId);
        return;
    }

    ALOGD("[halimp_aidl] hot plug port id %x, is connected %x", (msgBuf[0] & 0xf),
          (msgBuf[3] & 0xf));
    mPortConnectionStatus.at(portId - 1) = connected;
    if (mPortInfos.at(portId - 1).type == HdmiPortType::OUTPUT) {
        mPhysicalAddress = (connected ? 0xffff : ((msgBuf[1] << 8) | (msgBuf[2])));
        mPortInfos.at(portId - 1).physicalAddress = mPhysicalAddress;
        ALOGD("[halimp_aidl] hot plug physical address %x", mPhysicalAddress);
    }

    if (mCallback != nullptr) {
        mCallback->onHotplugEvent(connected, portId);
    }
}

void HdmiConnectionMock::threadLoop() {
    ALOGD("[halimp_aidl] threadLoop start.");
    unsigned char msgBuf[MESSAGE_BODY_MAX_LENGTH];
    int r = -1;

    // Open the input pipe
    while (mHdmiThreadRun && mInputFile < 0) {
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

HdmiConnectionMock::HdmiConnectionMock() {
    ALOGE("[halimp_aidl] Opening a virtual HDMI HAL for testing and virtual machine.");
    mCallback = nullptr;
    mPortInfos.resize(mTotalPorts);
    mPortConnectionStatus.resize(mTotalPorts);
    mHpdSignal.resize(mTotalPorts);
    mPortInfos[0] = {.type = HdmiPortType::OUTPUT,
                     .portId = static_cast<uint32_t>(1),
                     .cecSupported = true,
                     .arcSupported = false,
                     .eArcSupported = false,
                     .physicalAddress = mPhysicalAddress};
    mPortConnectionStatus[0] = false;
    mHpdSignal[0] = HpdSignal::HDMI_HPD_PHYSICAL;
    mDeathRecipient = ndk::ScopedAIBinder_DeathRecipient(nullptr);
}

void HdmiConnectionMock::stopThread() {
    if (mCallback != nullptr) {
        ALOGE("[halimp_aidl] HdmiConnectionMock shutting down.");
        mCallback = nullptr;
        mDeathRecipient = ndk::ScopedAIBinder_DeathRecipient(nullptr);
        mHdmiThreadRun = false;
        pthread_join(mThreadId, NULL);
    }
}

HdmiConnectionMock::~HdmiConnectionMock() {
    stopThread();
}

}  // namespace implementation
}  // namespace connection
}  // namespace hdmi
}  // namespace tv
}  // namespace hardware
}  // namespace android
