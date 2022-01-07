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

#include <Utils.h>
#include <android/hardware/gnss/IGnss.h>
#include "gtest/gtest.h"

#include <cutils/properties.h>

namespace android {
namespace hardware {
namespace gnss {
namespace common {

using namespace measurement_corrections::V1_0;
using V1_0::GnssLocationFlags;

template <>
int64_t Utils::getLocationTimestampMillis(const android::hardware::gnss::GnssLocation& location) {
    return location.timestampMillis;
}

template <>
int64_t Utils::getLocationTimestampMillis(const V1_0::GnssLocation& location) {
    return location.timestamp;
}

const MeasurementCorrections Utils::getMockMeasurementCorrections() {
    ReflectingPlane reflectingPlane = {
            .latitudeDegrees = 37.4220039,
            .longitudeDegrees = -122.0840991,
            .altitudeMeters = 250.35,
            .azimuthDegrees = 203.0,
    };

    SingleSatCorrection singleSatCorrection1 = {
            .singleSatCorrectionFlags = GnssSingleSatCorrectionFlags::HAS_SAT_IS_LOS_PROBABILITY |
                                        GnssSingleSatCorrectionFlags::HAS_EXCESS_PATH_LENGTH |
                                        GnssSingleSatCorrectionFlags::HAS_EXCESS_PATH_LENGTH_UNC |
                                        GnssSingleSatCorrectionFlags::HAS_REFLECTING_PLANE,
            .constellation = V1_0::GnssConstellationType::GPS,
            .svid = 12,
            .carrierFrequencyHz = 1.59975e+09,
            .probSatIsLos = 0.50001,
            .excessPathLengthMeters = 137.4802,
            .excessPathLengthUncertaintyMeters = 25.5,
            .reflectingPlane = reflectingPlane,
    };
    SingleSatCorrection singleSatCorrection2 = {
            .singleSatCorrectionFlags = GnssSingleSatCorrectionFlags::HAS_SAT_IS_LOS_PROBABILITY |
                                        GnssSingleSatCorrectionFlags::HAS_EXCESS_PATH_LENGTH |
                                        GnssSingleSatCorrectionFlags::HAS_EXCESS_PATH_LENGTH_UNC,
            .constellation = V1_0::GnssConstellationType::GPS,
            .svid = 9,
            .carrierFrequencyHz = 1.59975e+09,
            .probSatIsLos = 0.873,
            .excessPathLengthMeters = 26.294,
            .excessPathLengthUncertaintyMeters = 10.0,
    };

    hidl_vec<SingleSatCorrection> singleSatCorrections = {singleSatCorrection1,
                                                          singleSatCorrection2};
    MeasurementCorrections mockCorrections = {
            .latitudeDegrees = 37.4219999,
            .longitudeDegrees = -122.0840575,
            .altitudeMeters = 30.60062531,
            .horizontalPositionUncertaintyMeters = 9.23542,
            .verticalPositionUncertaintyMeters = 15.02341,
            .toaGpsNanosecondsOfWeek = 2935633453L,
            .satCorrections = singleSatCorrections,
    };
    return mockCorrections;
}

const measurement_corrections::V1_1::MeasurementCorrections
Utils::getMockMeasurementCorrections_1_1() {
    MeasurementCorrections mockCorrections_1_0 = getMockMeasurementCorrections();

    measurement_corrections::V1_1::SingleSatCorrection singleSatCorrection1 = {
            .v1_0 = mockCorrections_1_0.satCorrections[0],
            .constellation = V2_0::GnssConstellationType::IRNSS,
    };
    measurement_corrections::V1_1::SingleSatCorrection singleSatCorrection2 = {
            .v1_0 = mockCorrections_1_0.satCorrections[1],
            .constellation = V2_0::GnssConstellationType::IRNSS,
    };

    mockCorrections_1_0.satCorrections[0].constellation = V1_0::GnssConstellationType::UNKNOWN;
    mockCorrections_1_0.satCorrections[1].constellation = V1_0::GnssConstellationType::UNKNOWN;

    hidl_vec<measurement_corrections::V1_1::SingleSatCorrection> singleSatCorrections = {
            singleSatCorrection1, singleSatCorrection2};

    measurement_corrections::V1_1::MeasurementCorrections mockCorrections_1_1 = {
            .v1_0 = mockCorrections_1_0,
            .hasEnvironmentBearing = true,
            .environmentBearingDegrees = 45.0,
            .environmentBearingUncertaintyDegrees = 4.0,
            .satCorrections = singleSatCorrections,
    };
    return mockCorrections_1_1;
}

/*
 * MapConstellationType:
 * Given a GnssConstellationType_2_0 type constellation, maps to its equivalent
 * GnssConstellationType_1_0 type constellation. For constellations that do not have
 * an equivalent value, maps to GnssConstellationType_1_0::UNKNOWN
 */
V1_0::GnssConstellationType Utils::mapConstellationType(V2_0::GnssConstellationType constellation) {
    switch (constellation) {
        case V2_0::GnssConstellationType::GPS:
            return V1_0::GnssConstellationType::GPS;
        case V2_0::GnssConstellationType::SBAS:
            return V1_0::GnssConstellationType::SBAS;
        case V2_0::GnssConstellationType::GLONASS:
            return V1_0::GnssConstellationType::GLONASS;
        case V2_0::GnssConstellationType::QZSS:
            return V1_0::GnssConstellationType::QZSS;
        case V2_0::GnssConstellationType::BEIDOU:
            return V1_0::GnssConstellationType::BEIDOU;
        case V2_0::GnssConstellationType::GALILEO:
            return V1_0::GnssConstellationType::GALILEO;
        default:
            return V1_0::GnssConstellationType::UNKNOWN;
    }
}

bool Utils::isAutomotiveDevice() {
    char buffer[PROPERTY_VALUE_MAX] = {0};
    property_get("ro.hardware.type", buffer, "");
    return strncmp(buffer, "automotive", PROPERTY_VALUE_MAX) == 0;
}

}  // namespace common
}  // namespace gnss
}  // namespace hardware
}  // namespace android
