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
#include <android/hardware/gnss/BnGnss.h>
#include <android/hardware/gnss/IGnss.h>
#include "gtest/gtest.h"

#include <cutils/properties.h>
#include <utils/SystemClock.h>

namespace android {
namespace hardware {
namespace gnss {
namespace common {

using android::hardware::gnss::ElapsedRealtime;
using android::hardware::gnss::GnssLocation;

using namespace measurement_corrections::V1_0;
using V1_0::GnssLocationFlags;

using MeasurementCorrectionsAidl =
        android::hardware::gnss::measurement_corrections::MeasurementCorrections;
using ReflectingPlaneAidl = android::hardware::gnss::measurement_corrections::ReflectingPlane;
using SingleSatCorrectionAidl =
        android::hardware::gnss::measurement_corrections::SingleSatCorrection;
using ExcessPathInfo = SingleSatCorrectionAidl::ExcessPathInfo;

template <>
int64_t Utils::getLocationTimestampMillis(const android::hardware::gnss::GnssLocation& location) {
    return location.timestampMillis;
}

template <>
int64_t Utils::getLocationTimestampMillis(const V1_0::GnssLocation& location) {
    return location.timestamp;
}

template <>
void Utils::checkLocationElapsedRealtime(const V1_0::GnssLocation&) {}

template <>
void Utils::checkLocationElapsedRealtime(const android::hardware::gnss::GnssLocation& location) {
    checkElapsedRealtime(location.elapsedRealtime);
}

void Utils::checkElapsedRealtime(const ElapsedRealtime& elapsedRealtime) {
    ASSERT_TRUE(elapsedRealtime.flags >= 0 &&
                elapsedRealtime.flags <= (ElapsedRealtime::HAS_TIMESTAMP_NS |
                                          ElapsedRealtime::HAS_TIME_UNCERTAINTY_NS));
    if (elapsedRealtime.flags & ElapsedRealtime::HAS_TIMESTAMP_NS) {
        ASSERT_TRUE(elapsedRealtime.timestampNs > 0);
    }
    if (elapsedRealtime.flags & ElapsedRealtime::HAS_TIME_UNCERTAINTY_NS) {
        ASSERT_TRUE(elapsedRealtime.timeUncertaintyNs > 0);
    }
}

const GnssLocation Utils::getMockLocation(double latitudeDegrees, double longitudeDegrees,
                                          double horizontalAccuracyMeters) {
    ElapsedRealtime elapsedRealtime;
    elapsedRealtime.flags =
            ElapsedRealtime::HAS_TIMESTAMP_NS | ElapsedRealtime::HAS_TIME_UNCERTAINTY_NS;
    elapsedRealtime.timestampNs = ::android::elapsedRealtimeNano();
    elapsedRealtime.timeUncertaintyNs = 1000;
    GnssLocation location;
    location.gnssLocationFlags = 0xFF;
    location.latitudeDegrees = latitudeDegrees;
    location.longitudeDegrees = longitudeDegrees;
    location.altitudeMeters = 500.0;
    location.speedMetersPerSec = 0.0;
    location.bearingDegrees = 0.0;
    location.horizontalAccuracyMeters = horizontalAccuracyMeters;
    location.verticalAccuracyMeters = 1000.0;
    location.speedAccuracyMetersPerSecond = 1000.0;
    location.bearingAccuracyDegrees = 90.0;
    location.timestampMillis =
            static_cast<int64_t>(kMockTimestamp + ::android::elapsedRealtimeNano() * 1e-6);
    location.elapsedRealtime = elapsedRealtime;
    return location;
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

namespace {
const ExcessPathInfo createExcessPathInfo(float excessPathLengthMeters,
                                          float excessPathLengthUncertaintyMeters,
                                          const ReflectingPlaneAidl* reflectingPlane,
                                          float attenuationDb) {
    ExcessPathInfo excessPathInfo;
    excessPathInfo.excessPathInfoFlags =
            ExcessPathInfo::EXCESS_PATH_INFO_HAS_EXCESS_PATH_LENGTH |
            ExcessPathInfo::EXCESS_PATH_INFO_HAS_EXCESS_PATH_LENGTH_UNC |
            ExcessPathInfo::EXCESS_PATH_INFO_HAS_ATTENUATION |
            (reflectingPlane == nullptr ? 0
                                        : ExcessPathInfo::EXCESS_PATH_INFO_HAS_REFLECTING_PLANE);
    excessPathInfo.excessPathLengthMeters = excessPathLengthMeters;
    excessPathInfo.excessPathLengthUncertaintyMeters = excessPathLengthUncertaintyMeters;
    if (reflectingPlane != nullptr) {
        excessPathInfo.reflectingPlane = *reflectingPlane;
    }
    excessPathInfo.attenuationDb = attenuationDb;
    return excessPathInfo;
}
}  // anonymous namespace

const MeasurementCorrectionsAidl Utils::getMockMeasurementCorrections_aidl() {
    ReflectingPlaneAidl reflectingPlane;
    reflectingPlane.latitudeDegrees = 37.4220039;
    reflectingPlane.longitudeDegrees = -122.0840991;
    reflectingPlane.altitudeMeters = 250.35;
    reflectingPlane.reflectingPlaneAzimuthDegrees = 203.0;

    SingleSatCorrectionAidl singleSatCorrection1;
    singleSatCorrection1.singleSatCorrectionFlags =
            SingleSatCorrectionAidl::SINGLE_SAT_CORRECTION_HAS_SAT_IS_LOS_PROBABILITY |
            SingleSatCorrectionAidl::SINGLE_SAT_CORRECTION_HAS_COMBINED_EXCESS_PATH_LENGTH |
            SingleSatCorrectionAidl::SINGLE_SAT_CORRECTION_HAS_COMBINED_EXCESS_PATH_LENGTH_UNC |
            SingleSatCorrectionAidl::SINGLE_SAT_CORRECTION_HAS_COMBINED_ATTENUATION;
    singleSatCorrection1.constellation = android::hardware::gnss::GnssConstellationType::GPS;
    singleSatCorrection1.svid = 12;
    singleSatCorrection1.carrierFrequencyHz = 1.59975e+09;
    singleSatCorrection1.probSatIsLos = 0.50001;
    singleSatCorrection1.combinedExcessPathLengthMeters = 203.5;
    singleSatCorrection1.combinedExcessPathLengthUncertaintyMeters = 59.1;
    singleSatCorrection1.combinedAttenuationDb = -4.3;
    singleSatCorrection1.excessPathInfos.push_back(
            createExcessPathInfo(137.4, 25.5, &reflectingPlane, -3.5));
    singleSatCorrection1.excessPathInfos.push_back(
            createExcessPathInfo(296.3, 87.2, &reflectingPlane, -5.1));

    SingleSatCorrectionAidl singleSatCorrection2;
    singleSatCorrection2.singleSatCorrectionFlags =
            SingleSatCorrectionAidl::SINGLE_SAT_CORRECTION_HAS_SAT_IS_LOS_PROBABILITY |
            SingleSatCorrectionAidl::SINGLE_SAT_CORRECTION_HAS_COMBINED_EXCESS_PATH_LENGTH |
            SingleSatCorrectionAidl::SINGLE_SAT_CORRECTION_HAS_COMBINED_EXCESS_PATH_LENGTH_UNC |
            SingleSatCorrectionAidl::SINGLE_SAT_CORRECTION_HAS_COMBINED_ATTENUATION;
    singleSatCorrection2.constellation = GnssConstellationType::GPS;
    singleSatCorrection2.svid = 9;
    singleSatCorrection2.carrierFrequencyHz = 1.59975e+09;
    singleSatCorrection2.probSatIsLos = 0.873;
    singleSatCorrection2.combinedExcessPathLengthMeters = 26.294;
    singleSatCorrection2.combinedExcessPathLengthUncertaintyMeters = 10.0;
    singleSatCorrection2.combinedAttenuationDb = -0.5;
    singleSatCorrection2.excessPathInfos.push_back(
            createExcessPathInfo(26.294, 10.0, nullptr, -0.5));

    std::vector<SingleSatCorrectionAidl> singleSatCorrections = {singleSatCorrection1,
                                                                 singleSatCorrection2};
    MeasurementCorrectionsAidl mockCorrections;
    mockCorrections.latitudeDegrees = 37.4219999;
    mockCorrections.longitudeDegrees = -122.0840575;
    mockCorrections.altitudeMeters = 30.60062531;
    mockCorrections.horizontalPositionUncertaintyMeters = 9.23542;
    mockCorrections.verticalPositionUncertaintyMeters = 15.02341;
    mockCorrections.toaGpsNanosecondsOfWeek = 2935633453L;
    mockCorrections.hasEnvironmentBearing = true;
    mockCorrections.environmentBearingDegrees = 45.0;
    mockCorrections.environmentBearingUncertaintyDegrees = 4.0;
    mockCorrections.satCorrections = singleSatCorrections;

    return mockCorrections;
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
