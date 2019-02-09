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

#include <Constants.h>
#include <Utils.h>

namespace android {
namespace hardware {
namespace gnss {
namespace common {

using GnssSvFlags = V1_0::IGnssCallback::GnssSvFlags;

GnssLocation Utils::getMockLocation() {
    GnssLocation location = {.gnssLocationFlags = 0xFF,
                             .latitudeDegrees = kMockLatitudeDegrees,
                             .longitudeDegrees = kMockLongitudeDegrees,
                             .altitudeMeters = kMockAltitudeMeters,
                             .speedMetersPerSec = kMockSpeedMetersPerSec,
                             .bearingDegrees = kMockBearingDegrees,
                             .horizontalAccuracyMeters = kMockHorizontalAccuracyMeters,
                             .verticalAccuracyMeters = kMockVerticalAccuracyMeters,
                             .speedAccuracyMetersPerSecond = kMockSpeedAccuracyMetersPerSecond,
                             .bearingAccuracyDegrees = kMockBearingAccuracyDegrees,
                             .timestamp = kMockTimestamp};
    return location;
}

GnssSvInfo Utils::getSvInfo(int16_t svid, GnssConstellationType type, float cN0DbHz,
                            float elevationDegrees, float azimuthDegrees) {
    GnssSvInfo svInfo = {.svid = svid,
                         .constellation = type,
                         .cN0Dbhz = cN0DbHz,
                         .elevationDegrees = elevationDegrees,
                         .azimuthDegrees = azimuthDegrees,
                         .svFlag = GnssSvFlags::USED_IN_FIX | GnssSvFlags::HAS_EPHEMERIS_DATA |
                                   GnssSvFlags::HAS_ALMANAC_DATA};
    return svInfo;
}

}  // namespace common
}  // namespace gnss
}  // namespace hardware
}  // namespace android
