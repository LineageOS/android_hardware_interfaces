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

#define LOG_TAG "GnssDebugAidl"

#include "GnssDebug.h"
#include <log/log.h>
#include "MockLocation.h"

namespace aidl::android::hardware::gnss {

ndk::ScopedAStatus GnssDebug::getDebugData(DebugData* debugData) {
    ALOGD("GnssDebug::getDebugData");

    PositionDebug positionDebug = {.valid = true,
                                   .latitudeDegrees = 37.4219999,
                                   .longitudeDegrees = -122.0840575,
                                   .altitudeMeters = 1.60062531,
                                   .speedMetersPerSec = 0,
                                   .bearingDegrees = 0,
                                   .horizontalAccuracyMeters = 5,
                                   .verticalAccuracyMeters = 5,
                                   .speedAccuracyMetersPerSecond = 1,
                                   .bearingAccuracyDegrees = 90,
                                   .ageSeconds = 0.99};
    TimeDebug timeDebug = {.timeEstimateMs = 1519930775453L,
                           .timeUncertaintyNs = 1000,
                           .frequencyUncertaintyNsPerSec = 5.0e4};
    std::vector<SatelliteData> satelliteDataArrayDebug = {};
    debugData->position = positionDebug;
    debugData->time = timeDebug;
    debugData->satelliteDataArray = satelliteDataArrayDebug;

    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::gnss
