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

namespace android {
namespace hardware {
namespace gnss {
namespace common {

using aidl::android::hardware::gnss::ElapsedRealtime;
using aidl::android::hardware::gnss::GnssLocation;

std::unique_ptr<GnssLocation> FixLocationParser::getLocationFromInputStr(
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
    ElapsedRealtime elapsedRealtime = {
            .flags = ElapsedRealtime::HAS_TIMESTAMP_NS | ElapsedRealtime::HAS_TIME_UNCERTAINTY_NS,
            .timestampNs = ::android::elapsedRealtimeNano(),
            // This is an hardcoded value indicating a 1ms of uncertainty between the two clocks.
            // In an actual implementation provide an estimate of the synchronization uncertainty
            // or don't set the field.
            .timeUncertaintyNs = 1020400};

    GnssLocation location = {
            .gnssLocationFlags = 0xFF,
            .latitudeDegrees = ParseUtils::tryParseDouble(locationValues[2], 0),
            .longitudeDegrees = ParseUtils::tryParseDouble(locationValues[3], 0),
            .altitudeMeters = ParseUtils::tryParseDouble(locationValues[4], 0),
            .speedMetersPerSec = ParseUtils::tryParseDouble(locationValues[5], 0),
            .bearingDegrees = ParseUtils::tryParseDouble(locationValues[7], 0),
            .horizontalAccuracyMeters = ParseUtils::tryParseDouble(locationValues[6], 0),
            .verticalAccuracyMeters = ParseUtils::tryParseDouble(locationValues[6], 0),
            .speedAccuracyMetersPerSecond = ParseUtils::tryParseDouble(locationValues[9], 0),
            .bearingAccuracyDegrees = ParseUtils::tryParseDouble(locationValues[10], 0),
            .timestampMillis = ParseUtils::tryParseLongLong(locationValues[8], 0),
            .elapsedRealtime = elapsedRealtime};
    return std::make_unique<GnssLocation>(location);
}

}  // namespace common
}  // namespace gnss
}  // namespace hardware
}  // namespace android