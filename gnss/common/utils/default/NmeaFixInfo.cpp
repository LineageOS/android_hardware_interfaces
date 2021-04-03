/*
 * Copyright (C) 2020 The Android Open Source Project
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

#define LOG_TAG "NmeaFixInfo"

#include <Constants.h>
#include <NmeaFixInfo.h>
#include <Utils.h>
#include <log/log.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utils/SystemClock.h>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace android {
namespace hardware {
namespace gnss {
namespace common {

NmeaFixInfo::NmeaFixInfo() : hasGMCRecord(false), hasGGARecord(false) {}

float NmeaFixInfo::getAltitudeMeters() const {
    return altitudeMeters;
}

float NmeaFixInfo::checkAndConvertToFloat(const std::string& sentence) {
    if (sentence.empty()) {
        return std::numeric_limits<float>::quiet_NaN();
    }
    return std::stof(sentence);
}

float NmeaFixInfo::getBearingAccuracyDegrees() const {
    // Current NMEA doesn't contains beaing accuracy inforamtion
    return kMockBearingAccuracyDegrees;
}
float NmeaFixInfo::getBearingDegrees() const {
    return bearingDegrees;
}

float NmeaFixInfo::getHorizontalAccuracyMeters() const {
    // Current NMEA doesn't contains horizontal accuracy inforamtion
    return kMockHorizontalAccuracyMeters;
}

float NmeaFixInfo::getLatDeg() const {
    return latDeg;
}

float NmeaFixInfo::getLngDeg() const {
    return lngDeg;
}

float NmeaFixInfo::getSpeedAccuracyMetersPerSecond() const {
    // Current NMEA doesn't contains speed accuracy inforamtion
    return kMockSpeedAccuracyMetersPerSecond;
}

float NmeaFixInfo::getSpeedMetersPerSec() const {
    return speedMetersPerSec;
}

int64_t NmeaFixInfo::getTimestamp() const {
    return timestamp;
}

float NmeaFixInfo::getVerticalAccuracyMeters() const {
    // Current NMEA doesn't contains vertical accuracy inforamtion
    return kMockVerticalAccuracyMeters;
}

int64_t NmeaFixInfo::nmeaPartsToTimestamp(const std::string& timeStr, const std::string& dateStr) {
    /**
     * In NMEA format, the full time can only get from the $GPRMC record, see
     * the following example:
     * $GPRMC,213204.00,A,3725.371240,N,12205.589239,W,000.0,000.0,290819,,,A*49
     * the datetime is stored in two parts, 213204 and 290819, which means
     * 2019/08/29 21:32:04, however for in unix the year starts from 1900, we
     * need to add the offset.
     */
    struct tm tm;
    const int32_t unixYearOffset = 100;
    tm.tm_mday = std::stoi(dateStr.substr(0, 2).c_str());
    tm.tm_mon = std::stoi(dateStr.substr(2, 2).c_str()) - 1;
    tm.tm_year = std::stoi(dateStr.substr(4, 2).c_str()) + unixYearOffset;
    tm.tm_hour = std::stoi(timeStr.substr(0, 2).c_str());
    tm.tm_min = std::stoi(timeStr.substr(2, 2).c_str());
    tm.tm_sec = std::stoi(timeStr.substr(4, 2).c_str());
    return static_cast<int64_t>(mktime(&tm) - timezone);
}

bool NmeaFixInfo::isValidFix() const {
    return hasGMCRecord && hasGGARecord;
}

void NmeaFixInfo::parseGGALine(const std::vector<std::string>& sentenceValues) {
    if (sentenceValues.size() == 0 || sentenceValues[0].compare(GPGA_RECORD_TAG) != 0) {
        return;
    }
    // LatDeg, need covert to degree, if it is 'N', should be negative value
    this->latDeg = std::stof(sentenceValues[2].substr(0, 2)) +
                   (std::stof(sentenceValues[2].substr(2)) / 60.0);
    if (sentenceValues[3].compare("N") != 0) {
        this->latDeg *= -1;
    }

    // LngDeg, need covert to degree, if it is 'E', should be negative value
    this->lngDeg = std::stof(sentenceValues[4].substr(0, 3)) +
                   std::stof(sentenceValues[4].substr(3)) / 60.0;
    if (sentenceValues[5].compare("E") != 0) {
        this->lngDeg *= -1;
    }

    this->altitudeMeters = std::stof(sentenceValues[9]);

    this->hDop = sentenceValues[8].empty() ? std::numeric_limits<float>::quiet_NaN()
                                           : std::stof(sentenceValues[8]);
    this->hasGGARecord = true;
}

void NmeaFixInfo::parseRMCLine(const std::vector<std::string>& sentenceValues) {
    if (sentenceValues.size() == 0 || sentenceValues[0].compare(GPRMC_RECORD_TAG) != 0) {
        return;
    }
    this->speedMetersPerSec = checkAndConvertToFloat(sentenceValues[7]);
    this->bearingDegrees = checkAndConvertToFloat(sentenceValues[8]);
    this->timestamp = nmeaPartsToTimestamp(sentenceValues[1], sentenceValues[9]);
    this->hasGMCRecord = true;
}

/** invalid the current NmeaFixInfo */
void NmeaFixInfo::reset() {
    this->altitudeMeters = 0;
    this->bearingDegrees = 0;
    this->fixId = 0;
    this->hasGMCRecord = false;
    this->hasGGARecord = false;
    this->latDeg = 0;
    this->lngDeg = 0;
    this->hDop = 0;
    this->vDop = 0;
    this->satelliteCount = 0;
    this->speedMetersPerSec = 0;
    this->timestamp = 0;
}

void NmeaFixInfo::splitStr(const std::string& line, const char& delimiter,
                           std::vector<std::string>& out) {
    std::istringstream iss(line);
    std::string item;
    while (std::getline(iss, item, delimiter)) {
        out.push_back(item);
    }
}

NmeaFixInfo& NmeaFixInfo::operator=(const NmeaFixInfo& rhs) {
    if (this == &rhs) return *this;
    this->altitudeMeters = rhs.altitudeMeters;
    this->bearingDegrees = rhs.bearingDegrees;
    this->fixId = rhs.fixId;
    this->hasGMCRecord = rhs.hasGMCRecord;
    this->hasGGARecord = rhs.hasGGARecord;
    this->hDop = rhs.hDop;
    this->vDop = rhs.vDop;
    this->latDeg = rhs.latDeg;
    this->lngDeg = rhs.lngDeg;
    this->satelliteCount = rhs.satelliteCount;
    this->speedMetersPerSec = rhs.speedMetersPerSec;
    this->timestamp = rhs.timestamp;

    return *this;
}

/**
 * Parses the input string in NMEA format and convert to GnssLocation.
 * Currently version only cares about $GPGGA and $GPRMC records. but we
 * can easily extend to other types supported by NMEA if needed.
 */
std::unique_ptr<V2_0::GnssLocation> NmeaFixInfo::getLocationFromInputStr(
        const std::string& inputStr) {
    std::vector<std::string> nmeaRecords;
    splitStr(inputStr, LINE_SEPARATOR, nmeaRecords);
    NmeaFixInfo nmeaFixInfo;
    NmeaFixInfo candidateFixInfo;
    uint32_t fixId = 0;
    double lastTimeStamp = 0;
    for (const auto& line : nmeaRecords) {
        if (line.compare(0, strlen(GPGA_RECORD_TAG), GPGA_RECORD_TAG) != 0 &&
            line.compare(0, strlen(GPRMC_RECORD_TAG), GPRMC_RECORD_TAG) != 0) {
            continue;
        }
        std::vector<std::string> sentenceValues;
        splitStr(line, COMMA_SEPARATOR, sentenceValues);
        if (sentenceValues.size() < MIN_COL_NUM) {
            continue;
        }
        double currentTimeStamp = std::stof(sentenceValues[1]);
        // If see a new timestamp, report correct location.
        if ((currentTimeStamp - lastTimeStamp) > TIMESTAMP_EPSILON &&
            candidateFixInfo.isValidFix()) {
            nmeaFixInfo = candidateFixInfo;
            candidateFixInfo.reset();
            fixId++;
        }
        if (line.compare(0, strlen(GPGA_RECORD_TAG), GPGA_RECORD_TAG) == 0) {
            candidateFixInfo.fixId = fixId;
            candidateFixInfo.parseGGALine(sentenceValues);
        } else if (line.compare(0, strlen(GPRMC_RECORD_TAG), GPRMC_RECORD_TAG) == 0) {
            candidateFixInfo.parseRMCLine(sentenceValues);
        }
    }
    if (candidateFixInfo.isValidFix()) {
        nmeaFixInfo = candidateFixInfo;
        candidateFixInfo.reset();
    }
    if (!nmeaFixInfo.isValidFix()) {
        return nullptr;
    }
    return nmeaFixInfo.toGnssLocation();
}

/**
 * Parses the input string in NMEA format and convert to GnssLocation.
 */
std::unique_ptr<V2_0::GnssLocation> NmeaFixInfo::toGnssLocation() const {
    const V2_0::ElapsedRealtime currentOsTimestamp = {
            .flags = V2_0::ElapsedRealtimeFlags::HAS_TIMESTAMP_NS |
                     V2_0::ElapsedRealtimeFlags::HAS_TIME_UNCERTAINTY_NS,
            .timestampNs = static_cast<uint64_t>(::android::elapsedRealtimeNano()),
            // This is an hardcoded value indicating a 1ms of uncertainty between the two clocks.
            // In an actual implementation provide an estimate of the synchronization uncertainty
            // or don't set the field.
            .timeUncertaintyNs = 1000000};

    V1_0::GnssLocation locationV1 = {
            .gnssLocationFlags = 0xFF,
            .latitudeDegrees = this->getLatDeg(),
            .longitudeDegrees = this->getLngDeg(),
            .altitudeMeters = this->getAltitudeMeters(),
            .speedMetersPerSec = this->getSpeedMetersPerSec(),
            .bearingDegrees = this->getBearingDegrees(),
            .horizontalAccuracyMeters = this->getHorizontalAccuracyMeters(),
            .verticalAccuracyMeters = this->getVerticalAccuracyMeters(),
            .speedAccuracyMetersPerSecond = this->getSpeedAccuracyMetersPerSecond(),
            .bearingAccuracyDegrees = this->getBearingAccuracyDegrees(),
            .timestamp = this->getTimestamp()};

    V2_0::GnssLocation locationV2 = {.v1_0 = locationV1, .elapsedRealtime = currentOsTimestamp};

    return std::make_unique<V2_0::GnssLocation>(locationV2);
}

}  // namespace common
}  // namespace gnss
}  // namespace hardware
}  // namespace android