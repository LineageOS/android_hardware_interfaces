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
#include <utils/SystemClock.h>

namespace android {
namespace hardware {
namespace gnss {
namespace common {

using GnssSvFlags = V1_0::IGnssCallback::GnssSvFlags;
using GnssMeasurementFlagsV1_0 = V1_0::IGnssMeasurementCallback::GnssMeasurementFlags;
using GnssMeasurementFlagsV2_1 = V2_1::IGnssMeasurementCallback::GnssMeasurementFlags;
using GnssMeasurementStateV2_0 = V2_0::IGnssMeasurementCallback::GnssMeasurementState;
using ElapsedRealtime = V2_0::ElapsedRealtime;
using ElapsedRealtimeFlags = V2_0::ElapsedRealtimeFlags;
using GnssConstellationTypeV2_0 = V2_0::GnssConstellationType;
using IGnssMeasurementCallbackV2_0 = V2_0::IGnssMeasurementCallback;
using GnssSignalType = V2_1::GnssSignalType;

GnssDataV2_1 Utils::getMockMeasurementV2_1() {
    GnssDataV2_0 gnssDataV2_0 = Utils::getMockMeasurementV2_0();
    V2_1::IGnssMeasurementCallback::GnssMeasurement gnssMeasurementV2_1 = {
            .v2_0 = gnssDataV2_0.measurements[0],
            .flags = (uint32_t)(GnssMeasurementFlagsV2_1::HAS_CARRIER_FREQUENCY |
                                GnssMeasurementFlagsV2_1::HAS_CARRIER_PHASE |
                                GnssMeasurementFlagsV2_1::HAS_RECEIVER_ISB |
                                GnssMeasurementFlagsV2_1::HAS_RECEIVER_ISB_UNCERTAINTY |
                                GnssMeasurementFlagsV2_1::HAS_SATELLITE_ISB |
                                GnssMeasurementFlagsV2_1::HAS_SATELLITE_ISB_UNCERTAINTY),
            .receiverInterSignalBiasNs = 10.0,
            .receiverInterSignalBiasUncertaintyNs = 100.0,
            .satelliteInterSignalBiasNs = 20.0,
            .satelliteInterSignalBiasUncertaintyNs = 150.0,
            .basebandCN0DbHz = 25.0,
    };
    GnssSignalType referenceSignalTypeForIsb = {
            .constellation = GnssConstellationTypeV2_0::GPS,
            .carrierFrequencyHz = 1.59975e+09,
            .codeType = "C",
    };
    V2_1::IGnssMeasurementCallback::GnssClock gnssClockV2_1 = {
            .v1_0 = gnssDataV2_0.clock,
            .referenceSignalTypeForIsb = referenceSignalTypeForIsb,
    };
    hidl_vec<V2_1::IGnssMeasurementCallback::GnssMeasurement> measurements(1);
    measurements[0] = gnssMeasurementV2_1;
    GnssDataV2_1 gnssDataV2_1 = {
            .measurements = measurements,
            .clock = gnssClockV2_1,
            .elapsedRealtime = gnssDataV2_0.elapsedRealtime,
    };
    return gnssDataV2_1;
}

GnssDataV2_0 Utils::getMockMeasurementV2_0() {
    V1_0::IGnssMeasurementCallback::GnssMeasurement measurement_1_0 = {
            .flags = (uint32_t)GnssMeasurementFlagsV1_0::HAS_CARRIER_FREQUENCY,
            .svid = (int16_t)6,
            .constellation = V1_0::GnssConstellationType::UNKNOWN,
            .timeOffsetNs = 0.0,
            .receivedSvTimeInNs = 8195997131077,
            .receivedSvTimeUncertaintyInNs = 15,
            .cN0DbHz = 30.0,
            .pseudorangeRateMps = -484.13739013671875,
            .pseudorangeRateUncertaintyMps = 1.0379999876022339,
            .accumulatedDeltaRangeState = (uint32_t)V1_0::IGnssMeasurementCallback::
                    GnssAccumulatedDeltaRangeState::ADR_STATE_UNKNOWN,
            .accumulatedDeltaRangeM = 0.0,
            .accumulatedDeltaRangeUncertaintyM = 0.0,
            .carrierFrequencyHz = 1.59975e+09,
            .multipathIndicator =
                    V1_0::IGnssMeasurementCallback::GnssMultipathIndicator::INDICATOR_UNKNOWN};
    V1_1::IGnssMeasurementCallback::GnssMeasurement measurement_1_1 = {.v1_0 = measurement_1_0};
    V2_0::IGnssMeasurementCallback::GnssMeasurement measurement_2_0 = {
            .v1_1 = measurement_1_1,
            .codeType = "C",
            .state = GnssMeasurementStateV2_0::STATE_CODE_LOCK |
                     GnssMeasurementStateV2_0::STATE_BIT_SYNC |
                     GnssMeasurementStateV2_0::STATE_SUBFRAME_SYNC |
                     GnssMeasurementStateV2_0::STATE_TOW_DECODED |
                     GnssMeasurementStateV2_0::STATE_GLO_STRING_SYNC |
                     GnssMeasurementStateV2_0::STATE_GLO_TOD_DECODED,
            .constellation = GnssConstellationTypeV2_0::GLONASS,
    };

    hidl_vec<IGnssMeasurementCallbackV2_0::GnssMeasurement> measurements(1);
    measurements[0] = measurement_2_0;
    V1_0::IGnssMeasurementCallback::GnssClock clock = {.timeNs = 2713545000000,
                                                       .fullBiasNs = -1226701900521857520,
                                                       .biasNs = 0.59689998626708984,
                                                       .biasUncertaintyNs = 47514.989972114563,
                                                       .driftNsps = -51.757811607455452,
                                                       .driftUncertaintyNsps = 310.64968328491528,
                                                       .hwClockDiscontinuityCount = 1};

    ElapsedRealtime timestamp = {
            .flags = ElapsedRealtimeFlags::HAS_TIMESTAMP_NS |
                     ElapsedRealtimeFlags::HAS_TIME_UNCERTAINTY_NS,
            .timestampNs = static_cast<uint64_t>(::android::elapsedRealtimeNano()),
            // This is an hardcoded value indicating a 1ms of uncertainty between the two clocks.
            // In an actual implementation provide an estimate of the synchronization uncertainty
            // or don't set the field.
            .timeUncertaintyNs = 1000000};

    GnssDataV2_0 gnssData = {
            .measurements = measurements, .clock = clock, .elapsedRealtime = timestamp};
    return gnssData;
}

V2_0::GnssLocation Utils::getMockLocationV2_0() {
    const V2_0::ElapsedRealtime timestamp = {
            .flags = V2_0::ElapsedRealtimeFlags::HAS_TIMESTAMP_NS |
                     V2_0::ElapsedRealtimeFlags::HAS_TIME_UNCERTAINTY_NS,
            .timestampNs = static_cast<uint64_t>(::android::elapsedRealtimeNano()),
            // This is an hardcoded value indicating a 1ms of uncertainty between the two clocks.
            // In an actual implementation provide an estimate of the synchronization uncertainty
            // or don't set the field.
            .timeUncertaintyNs = 1000000};

    V2_0::GnssLocation location = {.v1_0 = Utils::getMockLocationV1_0(),
                                   .elapsedRealtime = timestamp};
    return location;
}

V1_0::GnssLocation Utils::getMockLocationV1_0() {
    V1_0::GnssLocation location = {
            .gnssLocationFlags = 0xFF,
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

hidl_vec<GnssSvInfoV2_1> Utils::getMockSvInfoListV2_1() {
    GnssSvInfoV1_0 gnssSvInfoV1_0 =
            Utils::getMockSvInfoV1_0(3, V1_0::GnssConstellationType::GPS, 32.5, 59.1, 166.5);
    GnssSvInfoV2_0 gnssSvInfoV2_0 =
            Utils::getMockSvInfoV2_0(gnssSvInfoV1_0, V2_0::GnssConstellationType::GPS);
    hidl_vec<GnssSvInfoV2_1> gnssSvInfoList = {
            Utils::getMockSvInfoV2_1(gnssSvInfoV2_0, 27.5),
            getMockSvInfoV2_1(
                    getMockSvInfoV2_0(getMockSvInfoV1_0(5, V1_0::GnssConstellationType::GPS, 27.0,
                                                        29.0, 56.5),
                                      V2_0::GnssConstellationType::GPS),
                    22.0),
            getMockSvInfoV2_1(
                    getMockSvInfoV2_0(getMockSvInfoV1_0(17, V1_0::GnssConstellationType::GPS, 30.5,
                                                        71.0, 77.0),
                                      V2_0::GnssConstellationType::GPS),
                    25.5),
            getMockSvInfoV2_1(
                    getMockSvInfoV2_0(getMockSvInfoV1_0(26, V1_0::GnssConstellationType::GPS, 24.1,
                                                        28.0, 253.0),
                                      V2_0::GnssConstellationType::GPS),
                    19.1),
            getMockSvInfoV2_1(
                    getMockSvInfoV2_0(getMockSvInfoV1_0(5, V1_0::GnssConstellationType::GLONASS,
                                                        20.5, 11.5, 116.0),
                                      V2_0::GnssConstellationType::GLONASS),
                    15.5),
            getMockSvInfoV2_1(
                    getMockSvInfoV2_0(getMockSvInfoV1_0(17, V1_0::GnssConstellationType::GLONASS,
                                                        21.5, 28.5, 186.0),
                                      V2_0::GnssConstellationType::GLONASS),
                    16.5),
            getMockSvInfoV2_1(
                    getMockSvInfoV2_0(getMockSvInfoV1_0(18, V1_0::GnssConstellationType::GLONASS,
                                                        28.3, 38.8, 69.0),
                                      V2_0::GnssConstellationType::GLONASS),
                    25.3),
            getMockSvInfoV2_1(
                    getMockSvInfoV2_0(getMockSvInfoV1_0(10, V1_0::GnssConstellationType::GLONASS,
                                                        25.0, 66.0, 247.0),
                                      V2_0::GnssConstellationType::GLONASS),
                    20.0),
            getMockSvInfoV2_1(
                    getMockSvInfoV2_0(getMockSvInfoV1_0(3, V1_0::GnssConstellationType::UNKNOWN,
                                                        22.0, 35.0, 112.0),
                                      V2_0::GnssConstellationType::IRNSS),
                    19.7),
    };
    return gnssSvInfoList;
}

GnssSvInfoV2_1 Utils::getMockSvInfoV2_1(GnssSvInfoV2_0 gnssSvInfoV2_0, float basebandCN0DbHz) {
    GnssSvInfoV2_1 gnssSvInfoV2_1 = {
            .v2_0 = gnssSvInfoV2_0,
            .basebandCN0DbHz = basebandCN0DbHz,
    };
    return gnssSvInfoV2_1;
}

GnssSvInfoV2_0 Utils::getMockSvInfoV2_0(GnssSvInfoV1_0 gnssSvInfoV1_0,
                                        V2_0::GnssConstellationType type) {
    GnssSvInfoV2_0 gnssSvInfoV2_0 = {
            .v1_0 = gnssSvInfoV1_0,
            .constellation = type,
    };
    return gnssSvInfoV2_0;
}

GnssSvInfoV1_0 Utils::getMockSvInfoV1_0(int16_t svid, V1_0::GnssConstellationType type,
                                        float cN0DbHz, float elevationDegrees,
                                        float azimuthDegrees) {
    GnssSvInfoV1_0 svInfo = {.svid = svid,
                             .constellation = type,
                             .cN0Dbhz = cN0DbHz,
                             .elevationDegrees = elevationDegrees,
                             .azimuthDegrees = azimuthDegrees,
                             .svFlag = GnssSvFlags::USED_IN_FIX | GnssSvFlags::HAS_EPHEMERIS_DATA |
                                       GnssSvFlags::HAS_ALMANAC_DATA};
    return svInfo;
}

hidl_vec<GnssAntennaInfo> Utils::getMockAntennaInfos() {
    GnssAntennaInfo mockAntennaInfo_1 = {
            .carrierFrequencyMHz = 123412.12,
            .phaseCenterOffsetCoordinateMillimeters = Coord{.x = 1,
                                                            .xUncertainty = 0.1,
                                                            .y = 2,
                                                            .yUncertainty = 0.1,
                                                            .z = 3,
                                                            .zUncertainty = 0.1},
            .phaseCenterVariationCorrectionMillimeters =
                    {
                            Row{hidl_vec<double>{1, -1, 5, -2, 3, -1}},
                            Row{hidl_vec<double>{-2, 3, 2, 0, 1, 2}},
                            Row{hidl_vec<double>{1, 3, 2, -1, -3, 5}},
                    },
            .phaseCenterVariationCorrectionUncertaintyMillimeters =
                    {
                            Row{hidl_vec<double>{0.1, 0.2, 0.4, 0.1, 0.2, 0.3}},
                            Row{hidl_vec<double>{0.3, 0.2, 0.3, 0.6, 0.1, 0.1}},
                            Row{hidl_vec<double>{0.1, 0.1, 0.4, 0.2, 0.5, 0.3}},
                    },
            .signalGainCorrectionDbi =
                    {
                            Row{hidl_vec<double>{2, -3, 1, -3, 0, -4}},
                            Row{hidl_vec<double>{1, 0, -4, 1, 3, -2}},
                            Row{hidl_vec<double>{3, -2, 0, -2, 3, 0}},
                    },
            .signalGainCorrectionUncertaintyDbi =
                    {
                            Row{hidl_vec<double>{0.3, 0.1, 0.2, 0.6, 0.1, 0.3}},
                            Row{hidl_vec<double>{0.1, 0.1, 0.5, 0.2, 0.3, 0.1}},
                            Row{hidl_vec<double>{0.2, 0.4, 0.2, 0.1, 0.1, 0.2}},
                    },
    };

    GnssAntennaInfo mockAntennaInfo_2 = {
            .carrierFrequencyMHz = 532324.23,
            .phaseCenterOffsetCoordinateMillimeters = Coord{.x = 5,
                                                            .xUncertainty = 0.1,
                                                            .y = 6,
                                                            .yUncertainty = 0.1,
                                                            .z = 7,
                                                            .zUncertainty = 0.1},
    };

    hidl_vec<GnssAntennaInfo> mockAntennaInfos = {
            mockAntennaInfo_1,
            mockAntennaInfo_2,
    };
    return mockAntennaInfos;
}

}  // namespace common
}  // namespace gnss
}  // namespace hardware
}  // namespace android
