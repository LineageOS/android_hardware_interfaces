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

#include "OccupantAwareness.h"

namespace android {
namespace hardware {
namespace automotive {
namespace occupant_awareness {
namespace V1_0 {
namespace implementation {

using ndk::ScopedAStatus;

static const int32_t kAllCapabilities = OccupantAwareness::CAP_PRESENCE_DETECTION |
                                        OccupantAwareness::CAP_GAZE_DETECTION |
                                        OccupantAwareness::CAP_DRIVER_MONITORING_DETECTION;

ScopedAStatus OccupantAwareness::startDetection(OccupantAwarenessStatus* status) {
    std::lock_guard<std::mutex> lock(mMutex);
    if (mStatus != OccupantAwarenessStatus::NOT_SUPPORTED) {
        mStatus = OccupantAwarenessStatus::NOT_SUPPORTED;
        if (mCallback) {
            mCallback->onSystemStatusChanged(kAllCapabilities,
                                             OccupantAwarenessStatus::NOT_SUPPORTED);
        }
    }
    *status = mStatus;
    return ScopedAStatus::ok();
}

ScopedAStatus OccupantAwareness::stopDetection(OccupantAwarenessStatus* status) {
    std::lock_guard<std::mutex> lock(mMutex);
    if (mStatus != OccupantAwarenessStatus::NOT_INITIALIZED) {
        mStatus = OccupantAwarenessStatus::NOT_INITIALIZED;
        if (mCallback) {
            mCallback->onSystemStatusChanged(kAllCapabilities,
                                             OccupantAwarenessStatus::NOT_INITIALIZED);
        }
    }
    *status = mStatus;
    return ScopedAStatus::ok();
}

ScopedAStatus OccupantAwareness::getCapabilityForRole(Role occupantRole, int32_t* capabilities) {
    if (!isValidRole(occupantRole)) {
        return ScopedAStatus::fromExceptionCode(EX_TRANSACTION_FAILED);
    }

    // No awareness capability for default HAL.
    *capabilities = 0;
    return ScopedAStatus::ok();
}

ScopedAStatus OccupantAwareness::getState(Role occupantRole, int detectionCapability,
                                          OccupantAwarenessStatus* status) {
    if (!isValidRole(occupantRole)) {
        return ScopedAStatus::fromExceptionCode(EX_TRANSACTION_FAILED);
    }

    if (!isValidDetectionCapabilities(detectionCapability) ||
        !isSingularCapability(detectionCapability)) {
        return ScopedAStatus::fromExceptionCode(EX_TRANSACTION_FAILED);
    }

    std::lock_guard<std::mutex> lock(mMutex);
    *status = mStatus;
    return ScopedAStatus::ok();
}

ScopedAStatus OccupantAwareness::setCallback(
        const std::shared_ptr<IOccupantAwarenessClientCallback>& callback) {
    if (callback == nullptr) {
        return ScopedAStatus::fromExceptionCode(EX_TRANSACTION_FAILED);
    }

    std::lock_guard<std::mutex> lock(mMutex);
    mCallback = callback;
    return ScopedAStatus::ok();
}

ScopedAStatus OccupantAwareness::getLatestDetection(OccupantDetections* detections) {
    // No detection generated for default hal.
    (void)detections;
    return ScopedAStatus::fromExceptionCode(EX_TRANSACTION_FAILED);
}

bool OccupantAwareness::isValidRole(Role occupantRole) {
    int intVal = static_cast<int>(occupantRole);
    int allOccupants = static_cast<int>(Role::ALL_OCCUPANTS);
    return (occupantRole != Role::INVALID) && ((intVal & (~allOccupants)) == 0);
}

bool OccupantAwareness::isValidDetectionCapabilities(int detectionCapabilities) {
    return (detectionCapabilities != CAP_NONE) &&
           ((detectionCapabilities & (~kAllCapabilities)) == 0);
}

bool OccupantAwareness::isSingularCapability(int detectionCapability) {
    // Check whether the value is 0, or the value has only one bit set.
    return (detectionCapability & (detectionCapability - 1)) == 0;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace occupant_awareness
}  // namespace automotive
}  // namespace hardware
}  // namespace android
