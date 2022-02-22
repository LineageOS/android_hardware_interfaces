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

#include "FixLocationParser.h"

#include <android/hardware/gnss/1.0/IGnss.h>

namespace android {
namespace hardware {
namespace gnss {
namespace common {

std::unique_ptr<V2_0::GnssLocation> FixLocationParser::getLocationFromInputStr(
        const std::string& locationStr) {
    /*
     * Fix,Provider,LatitudeDegrees,LongitudeDegrees,AltitudeMeters,SpeedMps,
     * AccuracyMeters,BearingDegrees,UnixTimeMillis,SpeedAccuracyMps,BearingAccuracyDegrees,
     * elapsedRealtimeNanos
     */
    if (locationStr.empty()) {
        return nullptr;
    }
    std::vector<std::string> locationStrRecords;
    ParseUtils::splitStr(locationStr, LINE_SEPARATOR, locationStrRecords);
    if (locationStrRecords.empty()) {
        return nullptr;
    }

    std::vector<std::string> locationValues;
    ParseUtils::splitStr(locationStrRecords[0], COMMA_SEPARATOR, locationValues);
    if (locationValues.size() < 12) {
        return nullptr;
    }
    V2_0::ElapsedRealtime elapsedRealtime = {
            .flags = V2_0::ElapsedRealtimeFlags::HAS_TIMESTAMP_NS |
                     V2_0::ElapsedRealtimeFlags::HAS_TIME_UNCERTAINTY_NS,
            .timestampNs = static_cast<uint64_t>(::android::elapsedRealtimeNano()),
            // This is an hardcoded value indicating a 1ms of uncertainty between the two clocks.
            // In an actual implementation provide an estimate of the synchronization uncertainty
            // or don't set the field.
            .timeUncertaintyNs = 1020400};

    V1_0::GnssLocation locationV1 = {
            .gnssLocationFlags = 0xFF,
            .latitudeDegrees = ParseUtils::tryParseDouble(locationValues[2], 0),
            .longitudeDegrees = ParseUtils::tryParseDouble(locationValues[3], 0),
            .altitudeMeters = ParseUtils::tryParseDouble(locationValues[4], 0),
            .speedMetersPerSec = ParseUtils::tryParsefloat(locationValues[5], 0),
            .bearingDegrees = ParseUtils::tryParsefloat(locationValues[7], 0),
            .horizontalAccuracyMeters = ParseUtils::tryParsefloat(locationValues[6], 0),
            .verticalAccuracyMeters = ParseUtils::tryParsefloat(locationValues[6], 0),
            .speedAccuracyMetersPerSecond = ParseUtils::tryParsefloat(locationValues[9], 0),
            .bearingAccuracyDegrees = ParseUtils::tryParsefloat(locationValues[10], 0),
            .timestamp = ParseUtils::tryParseLongLong(locationValues[8], 0)};

    V2_0::GnssLocation locationV2 = {.v1_0 = locationV1, .elapsedRealtime = elapsedRealtime};
    return std::make_unique<V2_0::GnssLocation>(locationV2);
}

}  // namespace common
}  // namespace gnss
}  // namespace hardware
}  // namespace android
