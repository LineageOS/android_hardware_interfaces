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

#define LOG_TAG "android.hardware.tv.hdmi.earc"
#include <android-base/logging.h>
#include <fcntl.h>
#include <utils/Log.h>

#include "EArcMock.h"

using ndk::ScopedAStatus;

namespace android {
namespace hardware {
namespace tv {
namespace hdmi {
namespace earc {
namespace implementation {

void EArcMock::serviceDied(void* cookie) {
    ALOGE("EArcMock died");
    auto eArc = static_cast<EArcMock*>(cookie);
    eArc->mEArcEnabled = false;
}

ScopedAStatus EArcMock::setEArcEnabled(bool in_enabled) {
    mEArcEnabled = in_enabled;
    if (mEArcEnabled != in_enabled) {
        return ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::FAILURE_UNKNOWN));
    } else {
        return ScopedAStatus::ok();
    }
}

ScopedAStatus EArcMock::isEArcEnabled(bool* _aidl_return) {
    *_aidl_return = mEArcEnabled;
    return ScopedAStatus::ok();
}

ScopedAStatus EArcMock::getState(int32_t portId, IEArcStatus* _aidl_return) {
    // Maintain port connection status and update on hotplug event
    if (portId <= mTotalPorts && portId >= 1) {
        *_aidl_return = mPortStatus.at(portId - 1);
    } else {
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    return ScopedAStatus::ok();
}

ScopedAStatus EArcMock::getLastReportedAudioCapabilities(int32_t portId,
                                                         std::vector<uint8_t>* _aidl_return) {
    if (portId <= mTotalPorts && portId >= 1) {
        *_aidl_return = mCapabilities.at(portId - 1);
    } else {
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    return ScopedAStatus::ok();
}

ScopedAStatus EArcMock::setCallback(const std::shared_ptr<IEArcCallback>& callback) {
    if (mCallback != nullptr) {
        mCallback = nullptr;
    }

    if (callback != nullptr) {
        mCallback = callback;
        AIBinder_linkToDeath(this->asBinder().get(), mDeathRecipient.get(), 0 /* cookie */);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus EArcMock::reportCapabilities(const std::vector<uint8_t>& capabilities,
                                           int32_t portId) {
    if (mCallback != nullptr) {
        mCallback->onCapabilitiesReported(capabilities, portId);
        return ScopedAStatus::ok();
    } else {
        return ScopedAStatus::fromExceptionCode(EX_NULL_POINTER);
    }
}

ScopedAStatus EArcMock::changeState(const IEArcStatus status, int32_t portId) {
    if (mCallback != nullptr) {
        mCallback->onStateChange(status, portId);
        return ScopedAStatus::ok();
    } else {
        return ScopedAStatus::fromExceptionCode(EX_NULL_POINTER);
    }
}

EArcMock::EArcMock() {
    ALOGE("[halimp_aidl] Opening a virtual eARC HAL for testing and virtual machine.");
    mCallback = nullptr;
    mCapabilities.resize(mTotalPorts);
    mPortStatus.resize(mTotalPorts);
    mPortStatus[0] = IEArcStatus::IDLE;
    mDeathRecipient = ndk::ScopedAIBinder_DeathRecipient(AIBinder_DeathRecipient_new(serviceDied));
}

}  // namespace implementation
}  // namespace earc
}  // namespace hdmi
}  // namespace tv
}  // namespace hardware
}  // namespace android
