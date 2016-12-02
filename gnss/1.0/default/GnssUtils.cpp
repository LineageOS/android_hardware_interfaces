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

#include "GnssUtils.h"

namespace android {
namespace hardware {
namespace gnss {
namespace V1_0 {
namespace implementation {

using android::hardware::gnss::V1_0::GnssLocation;

GnssLocation convertToGnssLocation(GpsLocation* location) {
    GnssLocation gnssLocation = {};
    if (location != nullptr) {
        gnssLocation = {
            .gnssLocationFlags = location->flags,
            .latitudeDegrees = location->latitude,
            .longitudeDegrees = location->longitude,
            .altitudeMeters = location->altitude,
            .speedMetersPerSec = location->speed,
            .bearingDegrees = location->bearing,
            .accuracyMeters = location->accuracy,
            .timestamp = location->timestamp
        };
    }

    return gnssLocation;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android
