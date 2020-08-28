/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "GnssDebug"

#include <log/log.h>

#include "Constants.h"
#include "GnssDebug.h"

using namespace ::android::hardware::gnss::common;

namespace android {
namespace hardware {
namespace gnss {
namespace V1_1 {
namespace implementation {

// Methods from ::android::hardware::gnss::V1_0::IGnssDebug follow.
Return<void> GnssDebug::getDebugData(V1_0::IGnssDebug::getDebugData_cb _hidl_cb) {
    PositionDebug positionDebug = {
            .valid = true,
            .latitudeDegrees = kMockLatitudeDegrees,
            .longitudeDegrees = kMockLongitudeDegrees,
            .altitudeMeters = kMockAltitudeMeters,
            .speedMetersPerSec = kMockSpeedMetersPerSec,
            .bearingDegrees = kMockBearingDegrees,
            .horizontalAccuracyMeters = kMockHorizontalAccuracyMeters,
            .verticalAccuracyMeters = kMockVerticalAccuracyMeters,
            .speedAccuracyMetersPerSecond = kMockSpeedAccuracyMetersPerSecond,
            .bearingAccuracyDegrees = kMockBearingAccuracyDegrees,
            .ageSeconds = 0.99};

    TimeDebug timeDebug = {.timeEstimate = kMockTimestamp,
                           .timeUncertaintyNs = 1000,
                           .frequencyUncertaintyNsPerSec = 5.0e4};

    DebugData data = {.position = positionDebug, .time = timeDebug};

    _hidl_cb(data);

    return Void();
}

}  // namespace implementation
}  // namespace V1_1
}  // namespace gnss
}  // namespace hardware
}  // namespace android
