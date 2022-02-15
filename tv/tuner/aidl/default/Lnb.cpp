/*
 * Copyright 2021 The Android Open Source Project
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

//#define LOG_NDEBUG 0
#define LOG_TAG "android.hardware.tv.tuner-service.example-Lnb"

#include "Lnb.h"
#include <utils/Log.h>

namespace aidl {
namespace android {
namespace hardware {
namespace tv {
namespace tuner {

Lnb::Lnb() {}

Lnb::Lnb(int id) {
    mId = id;
}

Lnb::~Lnb() {}

::ndk::ScopedAStatus Lnb::setCallback(const std::shared_ptr<ILnbCallback>& in_callback) {
    ALOGV("%s", __FUNCTION__);

    mCallback = in_callback;

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Lnb::setVoltage(LnbVoltage /* in_voltage */) {
    ALOGV("%s", __FUNCTION__);

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Lnb::setTone(LnbTone /* in_tone */) {
    ALOGV("%s", __FUNCTION__);

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Lnb::setSatellitePosition(LnbPosition /* in_position */) {
    ALOGV("%s", __FUNCTION__);

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Lnb::sendDiseqcMessage(const std::vector<uint8_t>& in_diseqcMessage) {
    ALOGV("%s", __FUNCTION__);

    if (mCallback != nullptr) {
        // The correct implementation should be to return the response from the
        // device via onDiseqcMessage(). The below implementation is only to enable
        // testing for LnbCallbacks.
        ALOGV("[aidl] %s - this is for test purpose only, and must be replaced!", __FUNCTION__);
        mCallback->onDiseqcMessage(in_diseqcMessage);
    }

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Lnb::close() {
    ALOGV("%s", __FUNCTION__);

    return ::ndk::ScopedAStatus::ok();
}

int Lnb::getId() {
    return mId;
}

}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
}  // namespace aidl
