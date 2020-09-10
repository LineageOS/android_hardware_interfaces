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

#include <utils/SystemClock.h>

#include "DetectionGenerator.h"

namespace android {
namespace hardware {
namespace automotive {
namespace occupant_awareness {
namespace V1_0 {
namespace implementation {

using ::aidl::android::hardware::automotive::occupant_awareness::ConfidenceLevel;
using ::aidl::android::hardware::automotive::occupant_awareness::DriverMonitoringDetection;
using ::aidl::android::hardware::automotive::occupant_awareness::OccupantDetection;
using ::aidl::android::hardware::automotive::occupant_awareness::PresenceDetection;

static int64_t kNanoSecondsPerMilliSecond = 1000 * 1000;

OccupantDetections DetectionGenerator::GetNextDetections() {
    OccupantDetections detections;
    detections.timeStampMillis = android::elapsedRealtimeNano() / kNanoSecondsPerMilliSecond;
    int remainingRoles = getSupportedRoles();
    while (remainingRoles) {
        int currentRole = remainingRoles & (~(remainingRoles - 1));
        remainingRoles = remainingRoles & (remainingRoles - 1);

        OccupantDetection occupantDetection;
        occupantDetection.role = static_cast<Role>(currentRole);

        // Add presence detection object for this occupant.
        PresenceDetection presenceDetection;
        presenceDetection.isOccupantDetected = true;
        presenceDetection.detectionDurationMillis = detections.timeStampMillis;
        occupantDetection.presenceData.emplace_back(presenceDetection);

        if (occupantDetection.role == Role::DRIVER) {
            // Add driver monitoring detection object for this occupant.
            DriverMonitoringDetection driverMonitoringDetection;
            driverMonitoringDetection.confidenceScore = ConfidenceLevel::HIGH;
            driverMonitoringDetection.isLookingOnRoad = 0;
            driverMonitoringDetection.gazeDurationMillis = detections.timeStampMillis;
            occupantDetection.attentionData.emplace_back(driverMonitoringDetection);
        }

        detections.detections.emplace_back(occupantDetection);
    }
    return detections;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace occupant_awareness
}  // namespace automotive
}  // namespace hardware
}  // namespace android
