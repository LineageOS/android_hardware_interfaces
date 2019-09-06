/*
 * Copyright (C) 2019 The Android Open Source Project
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

#define LOG_TAG "android.hardware.tv.cec@1.0-mock"
#include <android-base/logging.h>
#include <utils/Log.h>

#include <hardware/hardware.h>
#include <hardware/hdmi_cec.h>
#include "HdmiCecMock.h"

namespace android {
namespace hardware {
namespace tv {
namespace cec {
namespace V1_0 {
namespace implementation {

/*
 * (*set_option)() passes flags controlling the way HDMI-CEC service works down
 * to HAL implementation. Those flags will be used in case the feature needs
 * update in HAL itself, firmware or microcontroller.
 */
void HdmiCecMock::cec_set_option(int flag, int value) {
    // maintain options and set them accordingly
    switch (flag) {
        case HDMI_OPTION_WAKEUP:
            mOptionWakeUp = value;
            break;
        case HDMI_OPTION_ENABLE_CEC:
            mOptionEnableCec = value;
            break;
        case HDMI_OPTION_SYSTEM_CEC_CONTROL:
            mOptionSystemCecControl = value;
            break;
        case HDMI_OPTION_SET_LANG:
            mOptionLanguage = value;
    }
}

// Methods from ::android::hardware::tv::cec::V1_0::IHdmiCec follow.
Return<Result> HdmiCecMock::addLogicalAddress(CecLogicalAddress addr) {
    // have a list to maintain logical addresses
    int size = mLogicalAddresses.size();
    mLogicalAddresses.resize(size + 1);
    mLogicalAddresses[size + 1] = addr;
    return Result::SUCCESS;
}

Return<void> HdmiCecMock::clearLogicalAddress() {
    // remove logical address from the list
    mLogicalAddresses = {};
    return Void();
}

Return<void> HdmiCecMock::getPhysicalAddress(getPhysicalAddress_cb _hidl_cb) {
    // maintain a physical address and return it
    // default 0xFFFF, update on hotplug event
    _hidl_cb(Result::SUCCESS, mPhysicalAddress);
    return Void();
}

Return<SendMessageResult> HdmiCecMock::sendMessage(const CecMessage& message) {
    if (message.body.size() == 0) {
        return SendMessageResult::NACK;
    }
    sendMessageToFifo(message);
    return SendMessageResult::SUCCESS;
}

Return<void> HdmiCecMock::setCallback(const sp<IHdmiCecCallback>& callback) {
    if (mCallback != nullptr) {
        mCallback = nullptr;
    }

    if (callback != nullptr) {
        mCallback = callback;
        mCallback->linkToDeath(this, 0 /*cookie*/);

        mInputFile = open(CEC_MSG_IN_FIFO, O_RDWR);
        mOutputFile = open(CEC_MSG_OUT_FIFO, O_RDWR);
        pthread_create(&mThreadId, NULL, __threadLoop, this);
        pthread_setname_np(mThreadId, "hdmi_cec_loop");
    }
    return Void();
}

Return<int32_t> HdmiCecMock::getCecVersion() {
    // maintain a cec version and return it
    return mCecVersion;
}

Return<uint32_t> HdmiCecMock::getVendorId() {
    return mCecVendorId;
}

Return<void> HdmiCecMock::getPortInfo(getPortInfo_cb _hidl_cb) {
    // TODO ready port info from device specific config
    _hidl_cb(mPortInfo);
    return Void();
}

Return<void> HdmiCecMock::setOption(OptionKey key, bool value) {
    cec_set_option(static_cast<int>(key), value ? 1 : 0);
    return Void();
}

Return<void> HdmiCecMock::setLanguage(const hidl_string& language) {
    if (language.size() != 3) {
        LOG(ERROR) << "Wrong language code: expected 3 letters, but it was " << language.size()
                   << ".";
        return Void();
    }
    // TODO validate if language is a valid language code
    const char* languageStr = language.c_str();
    int convertedLanguage = ((languageStr[0] & 0xFF) << 16) | ((languageStr[1] & 0xFF) << 8) |
                            (languageStr[2] & 0xFF);
    cec_set_option(HDMI_OPTION_SET_LANG, convertedLanguage);
    return Void();
}

Return<void> HdmiCecMock::enableAudioReturnChannel(int32_t portId __unused, bool enable __unused) {
    // Maintain ARC status
    return Void();
}

Return<bool> HdmiCecMock::isConnected(int32_t portId) {
    // maintain port connection status and update on hotplug event
    if (portId < mTotalPorts && portId >= 0) {
        return mPortConnectionStatus[portId];
    }
    return false;
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
    /* maybe blocked at driver */
    ret = read(mInputFile, buf, msgCount);
    if (ret < 0) {
        ALOGE("[halimp] read :%s failed, ret:%d\n", CEC_MSG_IN_FIFO, ret);
        return -1;
    }

    return ret;
}

int HdmiCecMock::sendMessageToFifo(const CecMessage& message) {
    unsigned char msgBuf[CEC_MESSAGE_BODY_MAX_LENGTH];
    int ret = -1;

    memset(msgBuf, 0, sizeof(msgBuf));
    msgBuf[0] = ((static_cast<uint8_t>(message.initiator) & 0xf) << 4) |
                (static_cast<uint8_t>(message.destination) & 0xf);

    size_t length = std::min(static_cast<size_t>(message.body.size()),
                             static_cast<size_t>(MaxLength::MESSAGE_BODY));
    for (size_t i = 0; i < length; ++i) {
        msgBuf[i + 1] = static_cast<unsigned char>(message.body[i]);
    }

    // open the output pipe for writing outgoing cec message
    mOutputFile = open(CEC_MSG_OUT_FIFO, O_WRONLY);
    if (mOutputFile < 0) {
        ALOGD("[halimp] file open failed for writing");
        return -1;
    }

    // write message into the output pipe
    ret = write(mOutputFile, msgBuf, length + 1);
    close(mOutputFile);
    if (ret < 0) {
        ALOGE("[halimp] write :%s failed, ret:%d\n", CEC_MSG_OUT_FIFO, ret);
        return -1;
    }
    return ret;
}

void HdmiCecMock::printCecMsgBuf(const char* msg_buf, int len) {
    char buf[64] = {};
    int i, size = 0;
    memset(buf, 0, sizeof(buf));
    for (i = 0; i < len; i++) {
        size += sprintf(buf + size, " %02x", msg_buf[i]);
    }
    ALOGD("[halimp] %s, msg:%s", __FUNCTION__, buf);
}

void HdmiCecMock::handleHotplugMessage(unsigned char* msgBuf) {
    HotplugEvent hotplugEvent{.connected = ((msgBuf[3]) & 0xf) > 0,
                              .portId = static_cast<uint32_t>(msgBuf[0] & 0xf)};

    if (hotplugEvent.portId >= mPortInfo.size()) {
        ALOGD("[halimp] ignore hot plug message, id %x does not exist", hotplugEvent.portId);
        return;
    }

    ALOGD("[halimp] hot plug port id %x, is connected %x", (msgBuf[0] & 0xf), (msgBuf[3] & 0xf));
    if (mPortInfo[hotplugEvent.portId].type == HdmiPortType::OUTPUT) {
        mPhysicalAddress =
                ((hotplugEvent.connected == 0) ? 0xffff : ((msgBuf[1] << 8) | (msgBuf[2])));
        mPortInfo[hotplugEvent.portId].physicalAddress = mPhysicalAddress;
        ALOGD("[halimp] hot plug physical address %x", mPhysicalAddress);
    }

    // todo update connection status

    if (mCallback != nullptr) {
        mCallback->onHotplugEvent(hotplugEvent);
    }
}

void HdmiCecMock::handleCecMessage(unsigned char* msgBuf, int megSize) {
    CecMessage message;
    size_t length = std::min(static_cast<size_t>(megSize - 1),
                             static_cast<size_t>(MaxLength::MESSAGE_BODY));
    message.body.resize(length);

    for (size_t i = 0; i < length; ++i) {
        message.body[i] = static_cast<uint8_t>(msgBuf[i + 1]);
        ALOGD("[halimp] msg body %x", message.body[i]);
    }

    message.initiator = static_cast<CecLogicalAddress>((msgBuf[0] >> 4) & 0xf);
    ALOGD("[halimp] msg init %x", message.initiator);
    message.destination = static_cast<CecLogicalAddress>((msgBuf[0] >> 0) & 0xf);
    ALOGD("[halimp] msg dest %x", message.destination);

    // messageValidateAndHandle(&event);

    if (mCallback != nullptr) {
        mCallback->onCecMessage(message);
    }
}

void HdmiCecMock::threadLoop() {
    ALOGD("[halimp] threadLoop start.");
    unsigned char msgBuf[CEC_MESSAGE_BODY_MAX_LENGTH];
    int r = -1;

    // open the input pipe
    while (mInputFile < 0) {
        usleep(1000 * 1000);
        mInputFile = open(CEC_MSG_IN_FIFO, O_RDONLY);
    }
    ALOGD("[halimp] file open ok, fd = %d.", mInputFile);

    while (mCecThreadRun) {
        if (!mOptionSystemCecControl) {
            usleep(1000 * 1000);
            continue;
        }

        memset(msgBuf, 0, sizeof(msgBuf));
        // try to get a message from dev.
        // echo -n -e '\x04\x83' >> /dev/cec
        r = readMessageFromFifo(msgBuf, CEC_MESSAGE_BODY_MAX_LENGTH);
        if (r <= 1) {
            // ignore received ping messages
            continue;
        }

        printCecMsgBuf((const char*)msgBuf, r);

        if (((msgBuf[0] >> 4) & 0xf) == 0xf) {
            // the message is a hotplug event
            handleHotplugMessage(msgBuf);
            continue;
        }

        handleCecMessage(msgBuf, r);
    }

    ALOGD("[halimp] thread end.");
    // mCecDevice.mExited = true;
}

HdmiCecMock::HdmiCecMock() {
    ALOGE("[halimp] Opening a virtual HAL for testing and virtual machine.");
    mCallback = nullptr;
    mPortInfo.resize(mTotalPorts);
    mPortConnectionStatus.resize(mTotalPorts);
    mPortInfo[0] = {.type = HdmiPortType::OUTPUT,
                    .portId = static_cast<uint32_t>(0),
                    .cecSupported = true,
                    .arcSupported = false,
                    .physicalAddress = mPhysicalAddress};
    mPortConnectionStatus[0] = false;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace cec
}  // namespace tv
}  // namespace hardware
}  // namespace android
