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

#define LOG_TAG "GnssNavigationMessageAidl"

#include "GnssNavigationMessageInterface.h"
#include <aidl/android/hardware/gnss/BnGnss.h>
#include <log/log.h>
#include "Utils.h"

namespace aidl::android::hardware::gnss {

using namespace ::android::hardware::gnss;
using GnssNavigationMessage = IGnssNavigationMessageCallback::GnssNavigationMessage;
using GnssNavigationMessageType = GnssNavigationMessage::GnssNavigationMessageType;

std::shared_ptr<IGnssNavigationMessageCallback> GnssNavigationMessageInterface::sCallback = nullptr;

GnssNavigationMessageInterface::GnssNavigationMessageInterface() : mMinIntervalMillis(1000) {}

GnssNavigationMessageInterface::~GnssNavigationMessageInterface() {
    stop();
}

ndk::ScopedAStatus GnssNavigationMessageInterface::setCallback(
        const std::shared_ptr<IGnssNavigationMessageCallback>& callback) {
    ALOGD("setCallback");
    std::unique_lock<std::mutex> lock(mMutex);
    sCallback = callback;
    start();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus GnssNavigationMessageInterface::close() {
    ALOGD("close");
    stop();
    std::unique_lock<std::mutex> lock(mMutex);
    sCallback = nullptr;
    return ndk::ScopedAStatus::ok();
}

void GnssNavigationMessageInterface::start() {
    ALOGD("start");
    mIsActive = true;
    mThread = std::thread([this]() {
        while (mIsActive == true) {
            GnssNavigationMessage message = {
                    .svid = 19,
                    .type = GnssNavigationMessageType::GPS_L1CA,
                    .status = GnssNavigationMessage::STATUS_PARITY_PASSED,
                    .messageId = 2,
                    .submessageId = 3,
                    .data = std::vector<uint8_t>(40, 0xF9),
            };
            this->reportMessage(message);
            std::this_thread::sleep_for(std::chrono::milliseconds(mMinIntervalMillis));
        }
    });
    mThread.detach();
}

void GnssNavigationMessageInterface::stop() {
    ALOGD("stop");
    mIsActive = false;
}

void GnssNavigationMessageInterface::reportMessage(const GnssNavigationMessage& message) {
    ALOGD("reportMessage()");
    std::shared_ptr<IGnssNavigationMessageCallback> callbackCopy;
    {
        std::unique_lock<std::mutex> lock(mMutex);
        if (sCallback == nullptr) {
            ALOGE("%s: GnssNavigationMessageInterface::sCallback is null.", __func__);
            return;
        }
        callbackCopy = sCallback;
    }
    callbackCopy->gnssNavigationMessageCb(message);
}

}  // namespace aidl::android::hardware::gnss
