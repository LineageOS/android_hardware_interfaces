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
#include "gtest/gtest.h"

namespace android {
namespace hardware {
namespace gnss {
namespace common {

using V1_0::GnssConstellationType;
using V1_0::GnssLocationFlags;

void Utils::checkLocation(const GnssLocation& location, bool check_speed,
                          bool check_more_accuracies) {
    EXPECT_TRUE(location.gnssLocationFlags & GnssLocationFlags::HAS_LAT_LONG);
    EXPECT_TRUE(location.gnssLocationFlags & GnssLocationFlags::HAS_ALTITUDE);
    if (check_speed) {
        EXPECT_TRUE(location.gnssLocationFlags & GnssLocationFlags::HAS_SPEED);
    }
    EXPECT_TRUE(location.gnssLocationFlags & GnssLocationFlags::HAS_HORIZONTAL_ACCURACY);
    // New uncertainties available in O must be provided,
    // at least when paired with modern hardware (2017+)
    if (check_more_accuracies) {
        EXPECT_TRUE(location.gnssLocationFlags & GnssLocationFlags::HAS_VERTICAL_ACCURACY);
        if (check_speed) {
            EXPECT_TRUE(location.gnssLocationFlags & GnssLocationFlags::HAS_SPEED_ACCURACY);
            if (location.gnssLocationFlags & GnssLocationFlags::HAS_BEARING) {
                EXPECT_TRUE(location.gnssLocationFlags & GnssLocationFlags::HAS_BEARING_ACCURACY);
            }
        }
    }
    EXPECT_GE(location.latitudeDegrees, -90.0);
    EXPECT_LE(location.latitudeDegrees, 90.0);
    EXPECT_GE(location.longitudeDegrees, -180.0);
    EXPECT_LE(location.longitudeDegrees, 180.0);
    EXPECT_GE(location.altitudeMeters, -1000.0);
    EXPECT_LE(location.altitudeMeters, 30000.0);
    if (check_speed) {
        EXPECT_GE(location.speedMetersPerSec, 0.0);
        EXPECT_LE(location.speedMetersPerSec, 5.0);  // VTS tests are stationary.

        // Non-zero speeds must be reported with an associated bearing
        if (location.speedMetersPerSec > 0.0) {
            EXPECT_TRUE(location.gnssLocationFlags & GnssLocationFlags::HAS_BEARING);
        }
    }

    /*
     * Tolerating some especially high values for accuracy estimate, in case of
     * first fix with especially poor geometry (happens occasionally)
     */
    EXPECT_GT(location.horizontalAccuracyMeters, 0.0);
    EXPECT_LE(location.horizontalAccuracyMeters, 250.0);

    /*
     * Some devices may define bearing as -180 to +180, others as 0 to 360.
     * Both are okay & understandable.
     */
    if (location.gnssLocationFlags & GnssLocationFlags::HAS_BEARING) {
        EXPECT_GE(location.bearingDegrees, -180.0);
        EXPECT_LE(location.bearingDegrees, 360.0);
    }
    if (location.gnssLocationFlags & GnssLocationFlags::HAS_VERTICAL_ACCURACY) {
        EXPECT_GT(location.verticalAccuracyMeters, 0.0);
        EXPECT_LE(location.verticalAccuracyMeters, 500.0);
    }
    if (location.gnssLocationFlags & GnssLocationFlags::HAS_SPEED_ACCURACY) {
        EXPECT_GT(location.speedAccuracyMetersPerSecond, 0.0);
        EXPECT_LE(location.speedAccuracyMetersPerSecond, 50.0);
    }
    if (location.gnssLocationFlags & GnssLocationFlags::HAS_BEARING_ACCURACY) {
        EXPECT_GT(location.bearingAccuracyDegrees, 0.0);
        EXPECT_LE(location.bearingAccuracyDegrees, 360.0);
    }

    // Check timestamp > 1.48e12 (47 years in msec - 1970->2017+)
    EXPECT_GT(location.timestamp, 1.48e12);
}

const MeasurementCorrections_1_0 Utils::getMockMeasurementCorrections() {
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
            .constellation = GnssConstellationType::GPS,
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
            .constellation = GnssConstellationType::GPS,
            .svid = 9,
            .carrierFrequencyHz = 1.59975e+09,
            .probSatIsLos = 0.873,
            .excessPathLengthMeters = 26.294,
            .excessPathLengthUncertaintyMeters = 10.0,
    };

    hidl_vec<SingleSatCorrection> singleSatCorrections = {singleSatCorrection1,
                                                          singleSatCorrection2};
    MeasurementCorrections_1_0 mockCorrections = {
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

const MeasurementCorrections_1_1 Utils::getMockMeasurementCorrections_1_1() {
    MeasurementCorrections_1_0 mockCorrections_1_0 = getMockMeasurementCorrections();

    MeasurementCorrections_1_1 mockCorrections_1_1 = {
            .v1_0 = mockCorrections_1_0,
            .hasEnvironmentBearing = true,
            .environmentBearingDegrees = 45.0,
            .environmentBearingUncertaintyDegrees = 4.0,
    };
    return mockCorrections_1_1;
}

}  // namespace common
}  // namespace gnss
}  // namespace hardware
}  // namespace android
