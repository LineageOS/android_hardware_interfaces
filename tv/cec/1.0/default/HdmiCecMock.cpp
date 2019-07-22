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
    return SendMessageResult::SUCCESS;
}

Return<void> HdmiCecMock::setCallback(const sp<IHdmiCecCallback>& callback) {
    if (mCallback != nullptr) {
        mCallback = nullptr;
    }

    if (callback != nullptr) {
        mCallback = callback;
        mCallback->linkToDeath(this, 0 /*cookie*/);
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
    vector<HdmiPortInfo> portInfos;
    // TODO ready port info from device specific config
    portInfos.resize(mTotalPorts);
    for (int i = 0; i < mTotalPorts; ++i) {
        portInfos[i] = {.type = HdmiPortType::INPUT,
                        .portId = static_cast<uint32_t>(i),
                        .cecSupported = true,
                        .arcSupported = (i == 0),
                        .physicalAddress = static_cast<uint16_t>(i << 12)};
    }
    _hidl_cb(portInfos);
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

Return<bool> HdmiCecMock::isConnected(int32_t portId __unused) {
    // maintain port connection status and update on hotplug event
    return false;
}

HdmiCecMock::HdmiCecMock() {
    ALOGE("Opening a virtual HAL for testing and virtual machine.");
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace cec
}  // namespace tv
}  // namespace hardware
}  // namespace android
