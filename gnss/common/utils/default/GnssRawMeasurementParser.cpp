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

#include "GnssRawMeasurementParser.h"

namespace android {
namespace hardware {
namespace gnss {
namespace common {

using aidl::android::hardware::gnss::ElapsedRealtime;
using aidl::android::hardware::gnss::GnssClock;
using aidl::android::hardware::gnss::GnssConstellationType;
using aidl::android::hardware::gnss::GnssData;
using aidl::android::hardware::gnss::GnssMeasurement;
using aidl::android::hardware::gnss::GnssMultipathIndicator;
using aidl::android::hardware::gnss::GnssSignalType;

using ParseUtils = ::android::hardware::gnss::common::ParseUtils;

std::unordered_map<std::string, int> GnssRawMeasurementParser::getColumnIdNameMappingFromHeader(
        const std::string& header) {
    std::vector<std::string> columnNames;
    std::unordered_map<std::string, int> columnNameIdMapping;
    std::string s = header;
    // Trim left spaces
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    // Trim right spaces
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); })
                    .base(),
            s.end());
    // Remove comment symbol, start from `Raw`.
    s = s.substr(s.find("Raw"));

    ParseUtils::splitStr(s, COMMA_SEPARATOR, columnNames);
    int columnId = 0;
    for (auto& name : columnNames) {
        columnNameIdMapping[name] = columnId++;
    }

    return columnNameIdMapping;
}

int GnssRawMeasurementParser::getClockFlags(
        const std::vector<std::string>& rawMeasurementRecordValues,
        const std::unordered_map<std::string, int>& columnNameIdMapping) {
    int clockFlags = 0;
    if (!rawMeasurementRecordValues[columnNameIdMapping.at("LeapSecond")].empty()) {
        clockFlags |= GnssClock::HAS_LEAP_SECOND;
    }
    if (!rawMeasurementRecordValues[columnNameIdMapping.at("FullBiasNanos")].empty()) {
        clockFlags |= GnssClock::HAS_FULL_BIAS;
    }
    if (!rawMeasurementRecordValues[columnNameIdMapping.at("BiasNanos")].empty()) {
        clockFlags |= GnssClock::HAS_BIAS;
    }
    if (!rawMeasurementRecordValues[columnNameIdMapping.at("BiasUncertaintyNanos")].empty()) {
        clockFlags |= GnssClock::HAS_BIAS_UNCERTAINTY;
    }
    if (!rawMeasurementRecordValues[columnNameIdMapping.at("DriftNanosPerSecond")].empty()) {
        clockFlags |= GnssClock::HAS_DRIFT;
    }
    if (!rawMeasurementRecordValues[columnNameIdMapping.at("DriftUncertaintyNanosPerSecond")]
                 .empty()) {
        clockFlags |= GnssClock::HAS_DRIFT_UNCERTAINTY;
    }
    return clockFlags;
}

int GnssRawMeasurementParser::getElapsedRealtimeFlags(
        const std::vector<std::string>& rawMeasurementRecordValues,
        const std::unordered_map<std::string, int>& columnNameIdMapping) {
    int elapsedRealtimeFlags = ElapsedRealtime::HAS_TIMESTAMP_NS;
    if (!rawMeasurementRecordValues[columnNameIdMapping.at("TimeUncertaintyNanos")].empty()) {
        elapsedRealtimeFlags |= ElapsedRealtime::HAS_TIME_UNCERTAINTY_NS;
    }
    return elapsedRealtimeFlags;
}

int GnssRawMeasurementParser::getRawMeasurementFlags(
        const std::vector<std::string>& rawMeasurementRecordValues,
        const std::unordered_map<std::string, int>& columnNameIdMapping) {
    int rawMeasurementFlags = 0;
    if (!rawMeasurementRecordValues[columnNameIdMapping.at("SnrInDb")].empty()) {
        rawMeasurementFlags |= GnssMeasurement::HAS_SNR;
    }
    if (!rawMeasurementRecordValues[columnNameIdMapping.at("CarrierFrequencyHz")].empty()) {
        rawMeasurementFlags |= GnssMeasurement::HAS_CARRIER_FREQUENCY;
    }
    if (!rawMeasurementRecordValues[columnNameIdMapping.at("CarrierCycles")].empty()) {
        rawMeasurementFlags |= GnssMeasurement::HAS_CARRIER_CYCLES;
    }
    if (!rawMeasurementRecordValues[columnNameIdMapping.at("CarrierPhase")].empty()) {
        rawMeasurementFlags |= GnssMeasurement::HAS_CARRIER_PHASE;
    }
    if (!rawMeasurementRecordValues[columnNameIdMapping.at("CarrierPhaseUncertainty")].empty()) {
        rawMeasurementFlags |= GnssMeasurement::HAS_CARRIER_PHASE_UNCERTAINTY;
    }
    if (!rawMeasurementRecordValues[columnNameIdMapping.at("AgcDb")].empty()) {
        rawMeasurementFlags |= GnssMeasurement::HAS_AUTOMATIC_GAIN_CONTROL;
    }
    if (!rawMeasurementRecordValues[columnNameIdMapping.at("FullInterSignalBiasNanos")].empty()) {
        rawMeasurementFlags |= GnssMeasurement::HAS_FULL_ISB;
    }
    if (!rawMeasurementRecordValues[columnNameIdMapping.at("FullInterSignalBiasUncertaintyNanos")]
                 .empty()) {
        rawMeasurementFlags |= GnssMeasurement::HAS_FULL_ISB_UNCERTAINTY;
    }
    if (!rawMeasurementRecordValues[columnNameIdMapping.at("SatelliteInterSignalBiasNanos")]
                 .empty()) {
        rawMeasurementFlags |= GnssMeasurement::HAS_SATELLITE_ISB;
    }
    if (!rawMeasurementRecordValues[columnNameIdMapping.at(
                                            "SatelliteInterSignalBiasUncertaintyNanos")]
                 .empty()) {
        rawMeasurementFlags |= GnssMeasurement::HAS_SATELLITE_ISB_UNCERTAINTY;
    }
    // HAS_SATELLITE_PVT and HAS_CORRELATION_VECTOR fields currently not in rawmeasurement
    // output, need add them later.
    return rawMeasurementFlags;
}

GnssConstellationType GnssRawMeasurementParser::getGnssConstellationType(int constellationType) {
    GnssConstellationType gnssConstellationType =
            aidl::android::hardware::gnss::GnssConstellationType::UNKNOWN;

    switch (constellationType) {
        case 1:
            gnssConstellationType = aidl::android::hardware::gnss::GnssConstellationType::GPS;
            break;
        case 2:
            gnssConstellationType = aidl::android::hardware::gnss::GnssConstellationType::SBAS;
            break;
        case 3:
            gnssConstellationType = aidl::android::hardware::gnss::GnssConstellationType::GLONASS;
            break;
        case 4:
            gnssConstellationType = aidl::android::hardware::gnss::GnssConstellationType::QZSS;
            break;
        case 5:
            gnssConstellationType = aidl::android::hardware::gnss::GnssConstellationType::BEIDOU;
            break;
        case 6:
            gnssConstellationType = aidl::android::hardware::gnss::GnssConstellationType::GALILEO;
            break;
        default:
            gnssConstellationType = aidl::android::hardware::gnss::GnssConstellationType::UNKNOWN;
    }

    return gnssConstellationType;
}

std::unique_ptr<GnssData> GnssRawMeasurementParser::getMeasurementFromStrs(
        std::string& rawMeasurementStr) {
    /*
     * Raw,utcTimeMillis,TimeNanos,LeapSecond,TimeUncertaintyNanos,FullBiasNanos,BiasNanos,
     * BiasUncertaintyNanos,DriftNanosPerSecond,DriftUncertaintyNanosPerSecond,
     * HardwareClockDiscontinuityCount,Svid,TimeOffsetNanos,State,ReceivedSvTimeNanos,
     * ReceivedSvTimeUncertaintyNanos,Cn0DbHz,PseudorangeRateMetersPerSecond,
     * PseudorangeRateUncertaintyMetersPerSecond,AccumulatedDeltaRangeState,
     * AccumulatedDeltaRangeMeters,AccumulatedDeltaRangeUncertaintyMeters,CarrierFrequencyHz,
     * CarrierCycles,CarrierPhase,CarrierPhaseUncertainty,MultipathIndicator,SnrInDb,
     * ConstellationType,AgcDb,BasebandCn0DbHz,FullInterSignalBiasNanos,
     * FullInterSignalBiasUncertaintyNanos,SatelliteInterSignalBiasNanos,
     * SatelliteInterSignalBiasUncertaintyNanos,CodeType,ChipsetElapsedRealtimeNanos
     */
    ALOGD("Parsing %zu bytes rawMeasurementStr.", rawMeasurementStr.size());
    if (rawMeasurementStr.empty()) {
        return nullptr;
    }
    std::vector<std::string> rawMeasurementStrRecords;
    ParseUtils::splitStr(rawMeasurementStr, LINE_SEPARATOR, rawMeasurementStrRecords);
    if (rawMeasurementStrRecords.size() <= 1) {
        ALOGE("Raw GNSS Measurements parser failed. (No records) ");
        return nullptr;
    }

    // Get the column name mapping from the header.
    std::unordered_map<std::string, int> columnNameIdMapping =
            getColumnIdNameMappingFromHeader(rawMeasurementStrRecords[0]);

    if (columnNameIdMapping.size() < 37 || !ParseUtils::isValidHeader(columnNameIdMapping)) {
        ALOGE("Raw GNSS Measurements parser failed. (No header or missing columns.) ");
        return nullptr;
    }

    // Set GnssClock from 1st record.
    std::size_t pointer = 1;
    std::vector<std::string> firstRecordValues;
    ParseUtils::splitStr(rawMeasurementStrRecords[pointer], COMMA_SEPARATOR, firstRecordValues);
    GnssClock clock = {
            .gnssClockFlags = getClockFlags(firstRecordValues, columnNameIdMapping),
            .timeNs = ParseUtils::tryParseLongLong(
                    firstRecordValues[columnNameIdMapping.at("TimeNanos")], 0),
            .fullBiasNs = ParseUtils::tryParseLongLong(
                    firstRecordValues[columnNameIdMapping.at("FullBiasNanos")], 0),
            .biasNs = ParseUtils::tryParseDouble(
                    firstRecordValues[columnNameIdMapping.at("BiasNanos")], 0),
            .biasUncertaintyNs = ParseUtils::tryParseDouble(
                    firstRecordValues[columnNameIdMapping.at("BiasUncertaintyNanos")], 0),
            .driftNsps = ParseUtils::tryParseDouble(
                    firstRecordValues[columnNameIdMapping.at("DriftNanosPerSecond")], 0),
            .driftUncertaintyNsps = ParseUtils::tryParseDouble(
                    firstRecordValues[columnNameIdMapping.at("DriftNanosPerSecond")], 0),
            .hwClockDiscontinuityCount = ParseUtils::tryParseInt(
                    firstRecordValues[columnNameIdMapping.at("HardwareClockDiscontinuityCount")],
                    0)};

    ElapsedRealtime timestamp = {
            .flags = getElapsedRealtimeFlags(firstRecordValues, columnNameIdMapping),
            .timestampNs = ParseUtils::tryParseLongLong(
                    firstRecordValues[columnNameIdMapping.at("ChipsetElapsedRealtimeNanos")]),
            .timeUncertaintyNs = ParseUtils::tryParseDouble(
                    firstRecordValues[columnNameIdMapping.at("TimeUncertaintyNanos")], 0)};

    std::vector<GnssMeasurement> measurementsVec;
    for (pointer = 1; pointer < rawMeasurementStrRecords.size(); pointer++) {
        std::vector<std::string> rawMeasurementValues;
        std::string line = rawMeasurementStrRecords[pointer];
        ParseUtils::splitStr(line, COMMA_SEPARATOR, rawMeasurementValues);
        GnssSignalType signalType = {
                .constellation = getGnssConstellationType(ParseUtils::tryParseInt(
                        rawMeasurementValues[columnNameIdMapping.at("ConstellationType")], 0)),
                .carrierFrequencyHz = ParseUtils::tryParseDouble(
                        rawMeasurementValues[columnNameIdMapping.at("CarrierFrequencyHz")], 0),
                .codeType = rawMeasurementValues[columnNameIdMapping.at("CodeType")],
        };
        GnssMeasurement measurement = {
                .flags = getRawMeasurementFlags(rawMeasurementValues, columnNameIdMapping),
                .svid = ParseUtils::tryParseInt(
                        rawMeasurementValues[columnNameIdMapping.at("Svid")], 0),
                .signalType = signalType,
                .receivedSvTimeInNs = ParseUtils::tryParseLongLong(
                        rawMeasurementValues[columnNameIdMapping.at("ReceivedSvTimeNanos")], 0),
                .receivedSvTimeUncertaintyInNs =
                        ParseUtils::tryParseLongLong(rawMeasurementValues[columnNameIdMapping.at(
                                                             "ReceivedSvTimeUncertaintyNanos")],
                                                     0),
                .antennaCN0DbHz = ParseUtils::tryParseDouble(
                        rawMeasurementValues[columnNameIdMapping.at("Cn0DbHz")], 0),
                .basebandCN0DbHz = ParseUtils::tryParseDouble(
                        rawMeasurementValues[columnNameIdMapping.at("BasebandCn0DbHz")], 0),
                .agcLevelDb = ParseUtils::tryParseDouble(
                        rawMeasurementValues[columnNameIdMapping.at("AgcDb")], 0),
                .pseudorangeRateMps =
                        ParseUtils::tryParseDouble(rawMeasurementValues[columnNameIdMapping.at(
                                                           "PseudorangeRateMetersPerSecond")],
                                                   0),
                .pseudorangeRateUncertaintyMps = ParseUtils::tryParseDouble(
                        rawMeasurementValues[columnNameIdMapping.at(
                                "PseudorangeRateUncertaintyMetersPerSecond")],
                        0),
                .accumulatedDeltaRangeState = ParseUtils::tryParseInt(
                        rawMeasurementValues[columnNameIdMapping.at("AccumulatedDeltaRangeState")],
                        0),
                .accumulatedDeltaRangeM = ParseUtils::tryParseDouble(
                        rawMeasurementValues[columnNameIdMapping.at("AccumulatedDeltaRangeMeters")],
                        0),
                .accumulatedDeltaRangeUncertaintyM = ParseUtils::tryParseDouble(
                        rawMeasurementValues[columnNameIdMapping.at(
                                "AccumulatedDeltaRangeUncertaintyMeters")],
                        0),
                .multipathIndicator = GnssMultipathIndicator::UNKNOWN,  // Not in GnssLogger yet.
                .state = ParseUtils::tryParseInt(
                        rawMeasurementValues[columnNameIdMapping.at("State")], 0),
                .fullInterSignalBiasNs = ParseUtils::tryParseDouble(rawMeasurementValues[31], 0),
                .fullInterSignalBiasUncertaintyNs = ParseUtils::tryParseDouble(
                        rawMeasurementValues[columnNameIdMapping.at("FullInterSignalBiasNanos")],
                        0),
                .satelliteInterSignalBiasNs =
                        ParseUtils::tryParseDouble(rawMeasurementValues[columnNameIdMapping.at(
                                                           "SatelliteInterSignalBiasNanos")],
                                                   0),
                .satelliteInterSignalBiasUncertaintyNs = ParseUtils::tryParseDouble(
                        rawMeasurementValues[columnNameIdMapping.at(
                                "SatelliteInterSignalBiasUncertaintyNanos")],
                        0),
                .satellitePvt = {},
                .correlationVectors = {}};
        measurementsVec.push_back(measurement);
    }

    GnssData gnssData = {
            .measurements = measurementsVec, .clock = clock, .elapsedRealtime = timestamp};
    return std::make_unique<GnssData>(gnssData);
}

}  // namespace common
}  // namespace gnss
}  // namespace hardware
}  // namespace android
