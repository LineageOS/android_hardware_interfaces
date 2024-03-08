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

#define LOG_TAG "android.hardware.tv.hdmi.cec"
#include <android-base/logging.h>
#include <fcntl.h>
#include <utils/Log.h>

#include <hardware/hardware.h>
#include <hardware/hdmi_cec.h>
#include "HdmiCecMock.h"

using ndk::ScopedAStatus;

namespace android {
namespace hardware {
namespace tv {
namespace hdmi {
namespace cec {
namespace implementation {

void HdmiCecMock::serviceDied(void* cookie) {
    ALOGE("HdmiCecMock died");
    auto hdmiCecMock = static_cast<HdmiCecMock*>(cookie);
    hdmiCecMock->mCecThreadRun = false;
    pthread_join(hdmiCecMock->mThreadId, NULL);
}

ScopedAStatus HdmiCecMock::addLogicalAddress(CecLogicalAddress addr, Result* _aidl_return) {
    // Have a list to maintain logical addresses
    mLogicalAddresses.push_back(addr);
    *_aidl_return = Result::SUCCESS;
    return ScopedAStatus::ok();
}

ScopedAStatus HdmiCecMock::clearLogicalAddress() {
    // Remove logical address from the list
    mLogicalAddresses = {};
    return ScopedAStatus::ok();
}

ScopedAStatus HdmiCecMock::enableAudioReturnChannel(int32_t portId __unused, bool enable __unused) {
    // Maintain ARC status
    return ScopedAStatus::ok();
}

ScopedAStatus HdmiCecMock::getCecVersion(int32_t* _aidl_return) {
    // Maintain a cec version and return it
    *_aidl_return = mCecVersion;
    return ScopedAStatus::ok();
}

ScopedAStatus HdmiCecMock::getPhysicalAddress(int32_t* _aidl_return) {
    // Maintain a physical address and return it
    // Default 0xFFFF, update on hotplug event
    *_aidl_return = mPhysicalAddress;
    return ScopedAStatus::ok();
}

ScopedAStatus HdmiCecMock::getVendorId(int32_t* _aidl_return) {
    *_aidl_return = mCecVendorId;
    return ScopedAStatus::ok();
}

ScopedAStatus HdmiCecMock::sendMessage(const CecMessage& message, SendMessageResult* _aidl_return) {
    if (message.body.size() == 0) {
        *_aidl_return = SendMessageResult::NACK;
    } else {
        sendMessageToFifo(message);
        *_aidl_return = SendMessageResult::SUCCESS;
    }
    return ScopedAStatus::ok();
}

ScopedAStatus HdmiCecMock::setCallback(const std::shared_ptr<IHdmiCecCallback>& callback) {
    // If callback is null, mCallback is also set to null so we do not call the old callback.
    mCallback = callback;

    if (callback != nullptr) {
        mDeathRecipient =
                ndk::ScopedAIBinder_DeathRecipient(AIBinder_DeathRecipient_new(serviceDied));
        AIBinder_linkToDeath(callback->asBinder().get(), mDeathRecipient.get(), this /* cookie */);

        mInputFile = open(CEC_MSG_IN_FIFO, O_RDWR | O_CLOEXEC);
        mOutputFile = open(CEC_MSG_OUT_FIFO, O_RDWR | O_CLOEXEC);
        pthread_create(&mThreadId, NULL, __threadLoop, this);
        pthread_setname_np(mThreadId, "hdmi_cec_loop");
    }
    return ScopedAStatus::ok();
}

ScopedAStatus HdmiCecMock::setLanguage(const std::string& language) {
    if (language.size() != 3) {
        LOG(ERROR) << "Wrong language code: expected 3 letters, but it was " << language.size()
                   << ".";
        return ScopedAStatus::ok();
    }
    // TODO Validate if language is a valid language code
    const char* languageStr = language.c_str();
    int convertedLanguage = ((languageStr[0] & 0xFF) << 16) | ((languageStr[1] & 0xFF) << 8) |
                            (languageStr[2] & 0xFF);
    mOptionLanguage = convertedLanguage;
    return ScopedAStatus::ok();
}

ScopedAStatus HdmiCecMock::enableWakeupByOtp(bool value) {
    mOptionWakeUp = value;
    return ScopedAStatus::ok();
}

ScopedAStatus HdmiCecMock::enableCec(bool value) {
    mOptionEnableCec = value;
    return ScopedAStatus::ok();
}

ScopedAStatus HdmiCecMock::enableSystemCecControl(bool value) {
    mOptionSystemCecControl = value;
    return ScopedAStatus::ok();
}

void* HdmiCecMock::__threadLoop(void* user) {
    HdmiCecMock* const self = static_cast<HdmiCecMock*>(user);
    self->threadLoop();
    return 0;
}

int HdmiCecMock::readMessageFromFifo(unsigned char* buf, int msgCount) {
    if (msgCount <= 0 || !buf) {
        return 0;
    }

    int ret = -1;
    // Maybe blocked at driver
    ret = read(mInputFile, buf, msgCount);
    if (ret < 0) {
        ALOGE("[halimp_aidl] read :%s failed, ret:%d\n", CEC_MSG_IN_FIFO, ret);
        return -1;
    }

    return ret;
}

int HdmiCecMock::sendMessageToFifo(const CecMessage& message) {
    unsigned char msgBuf[CEC_MESSAGE_BODY_MAX_LENGTH + 1] = {0};
    int ret = -1;

    msgBuf[0] = ((static_cast<uint8_t>(message.initiator) & 0xf) << 4) |
                (static_cast<uint8_t>(message.destination) & 0xf);

    size_t length = std::min(static_cast<size_t>(message.body.size()),
                             static_cast<size_t>(CEC_MESSAGE_BODY_MAX_LENGTH));
    for (size_t i = 0; i < length; ++i) {
        msgBuf[i + 1] = static_cast<unsigned char>(message.body[i]);
    }

    // Open the output pipe for writing outgoing cec message
    mOutputFile = open(CEC_MSG_OUT_FIFO, O_WRONLY | O_CLOEXEC);
    if (mOutputFile < 0) {
        ALOGD("[halimp_aidl] file open failed for writing");
        return -1;
    }

    // Write message into the output pipe
    ret = write(mOutputFile, msgBuf, length + 1);
    close(mOutputFile);
    if (ret < 0) {
        ALOGE("[halimp_aidl] write :%s failed, ret:%d\n", CEC_MSG_OUT_FIFO, ret);
        return -1;
    }
    return ret;
}

void HdmiCecMock::printCecMsgBuf(const char* msg_buf, int len) {
    int i, size = 0;
    const int bufSize = CEC_MESSAGE_BODY_MAX_LENGTH * 3;
    // Use 2 characters for each byte in the message plus 1 space
    char buf[bufSize] = {0};

    // Messages longer than max length will be truncated.
    for (i = 0; i < len && size < bufSize; i++) {
        size += sprintf(buf + size, " %02x", msg_buf[i]);
    }
    ALOGD("[halimp_aidl] %s, msg:%.*s", __FUNCTION__, size, buf);
}

void HdmiCecMock::handleCecMessage(unsigned char* msgBuf, int msgSize) {
    CecMessage message;
    size_t length = std::min(static_cast<size_t>(msgSize - 1),
                             static_cast<size_t>(CEC_MESSAGE_BODY_MAX_LENGTH));
    message.body.resize(length);

    for (size_t i = 0; i < length; ++i) {
        message.body[i] = static_cast<uint8_t>(msgBuf[i + 1]);
        ALOGD("[halimp_aidl] msg body %x", message.body[i]);
    }

    message.initiator = static_cast<CecLogicalAddress>((msgBuf[0] >> 4) & 0xf);
    ALOGD("[halimp_aidl] msg init %hhd", message.initiator);
    message.destination = static_cast<CecLogicalAddress>((msgBuf[0] >> 0) & 0xf);
    ALOGD("[halimp_aidl] msg dest %hhd", message.destination);

    if (mCallback != nullptr) {
        mCallback->onCecMessage(message);
    }
}

void HdmiCecMock::threadLoop() {
    ALOGD("[halimp_aidl] threadLoop start.");
    unsigned char msgBuf[CEC_MESSAGE_BODY_MAX_LENGTH];
    int r = -1;

    // Open the input pipe
    while (mCecThreadRun && mInputFile < 0) {
        usleep(1000 * 1000);
        mInputFile = open(CEC_MSG_IN_FIFO, O_RDONLY | O_CLOEXEC);
    }
    ALOGD("[halimp_aidl] file open ok, fd = %d.", mInputFile);

    while (mCecThreadRun) {
        if (!mOptionSystemCecControl) {
            usleep(1000 * 1000);
            continue;
        }

        memset(msgBuf, 0, sizeof(msgBuf));
        // Try to get a message from dev.
        // echo -n -e '\x04\x83' >> /dev/cec
        r = readMessageFromFifo(msgBuf, CEC_MESSAGE_BODY_MAX_LENGTH);
        if (r <= 1) {
            // Ignore received ping messages
            continue;
        }

        printCecMsgBuf((const char*)msgBuf, r);

        if (((msgBuf[0] >> 4) & 0xf) == 0xf) {
            // The message is a hotplug event, handled by HDMI HAL.
            continue;
        }

        handleCecMessage(msgBuf, r);
    }

    ALOGD("[halimp_aidl] thread end.");
}

HdmiCecMock::HdmiCecMock() {
    ALOGE("[halimp_aidl] Opening a virtual CEC HAL for testing and virtual machine.");
    mCallback = nullptr;
    mDeathRecipient = ndk::ScopedAIBinder_DeathRecipient(nullptr);
}

HdmiCecMock::~HdmiCecMock() {
    ALOGE("[halimp_aidl] HdmiCecMock shutting down.");
    mCallback = nullptr;
    mDeathRecipient = ndk::ScopedAIBinder_DeathRecipient(nullptr);
    mCecThreadRun = false;
    pthread_join(mThreadId, NULL);
}

}  // namespace implementation
}  // namespace cec
}  // namespace hdmi
}  // namespace tv
}  // namespace hardware
}  // namespace android
