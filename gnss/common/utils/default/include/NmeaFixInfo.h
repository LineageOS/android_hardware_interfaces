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

#pragma once

#include <Constants.h>
#include <android/hardware/gnss/1.0/IGnss.h>
#include <android/hardware/gnss/2.0/IGnss.h>
#include <hidl/Status.h>
#include <ctime>
#include <string>
namespace android {
namespace hardware {
namespace gnss {
namespace common {

constexpr char GPGA_RECORD_TAG[] = "$GPGGA";
constexpr char GPRMC_RECORD_TAG[] = "$GPRMC";
constexpr char LINE_SEPARATOR = '\n';
constexpr char COMMA_SEPARATOR = ',';
constexpr double TIMESTAMP_EPSILON = 0.001;
constexpr int MIN_COL_NUM = 13;

/** Helper class to parse and store the GNSS fix details information. */
class NmeaFixInfo {
  private:
    float altitudeMeters;
    float bearingDegrees;
    uint32_t fixId;
    bool hasGMCRecord;
    bool hasGGARecord;
    float hDop;
    float vDop;
    float latDeg;
    float lngDeg;
    uint32_t satelliteCount;
    float speedMetersPerSec;
    int64_t timestamp;

  public:
    static std::unique_ptr<V2_0::GnssLocation> getLocationFromInputStr(const std::string& inputStr);

  private:
    static void splitStr(const std::string& line, const char& delimiter,
                         std::vector<std::string>& out);
    static float checkAndConvertToFloat(const std::string& sentence);
    static int64_t nmeaPartsToTimestamp(const std::string& timeStr, const std::string& dateStr);

    NmeaFixInfo();
    void parseGGALine(const std::vector<std::string>& sentenceValues);
    void parseRMCLine(const std::vector<std::string>& sentenceValues);
    std::unique_ptr<V2_0::GnssLocation> toGnssLocation() const;

    // Getters
    float getAltitudeMeters() const;
    float getBearingAccuracyDegrees() const;
    float getBearingDegrees() const;
    uint32_t getFixId() const;
    float getHorizontalAccuracyMeters() const;
    float getLatDeg() const;
    float getLngDeg() const;
    float getSpeedAccuracyMetersPerSecond() const;
    float getSpeedMetersPerSec() const;
    int64_t getTimestamp() const;
    float getVerticalAccuracyMeters() const;

    bool isValidFix() const;
    void reset();
    NmeaFixInfo& operator=(const NmeaFixInfo& rhs);
};

}  // namespace common
}  // namespace gnss
}  // namespace hardware
}  // namespace android