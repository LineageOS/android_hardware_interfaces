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

#include <ParseUtils.h>
#include <sstream>
#include <stdexcept>

namespace android {
namespace hardware {
namespace gnss {
namespace common {

int ParseUtils::tryParseInt(const std::string& s, int defaultVal) {
    if (s.empty()) {
        return defaultVal;
    } else {
        return std::stoi(s);
    }
}

float ParseUtils::tryParsefloat(const std::string& s, float defaultVal) {
    if (s.empty()) {
        return defaultVal;
    } else {
        return std::stof(s);
    }
}

double ParseUtils::tryParseDouble(const std::string& s, double defaultVal) {
    if (s.empty()) {
        return defaultVal;
    } else {
        return std::stod(s);
    }
}

long ParseUtils::tryParseLong(const std::string& s, long defaultVal) {
    if (s.empty()) {
        return defaultVal;
    } else {
        return std::stol(s);
    }
}

long long ParseUtils::tryParseLongLong(const std::string& s, long long defaultVal) {
    if (s.empty()) {
        return defaultVal;
    } else {
        return std::stoll(s);
    }
}

void ParseUtils::splitStr(const std::string& line, const char& delimiter,
                          std::vector<std::string>& out) {
    std::istringstream iss(line);
    std::string item;
    while (std::getline(iss, item, delimiter)) {
        out.push_back(item);
    }
}

bool ParseUtils::isValidHeader(const std::unordered_map<std::string, int>& columnNameIdMapping) {
    std::vector<std::string> requiredHeaderColumns = {"Raw",
                                                      "utcTimeMillis",
                                                      "TimeNanos",
                                                      "LeapSecond",
                                                      "TimeUncertaintyNanos",
                                                      "FullBiasNanos",
                                                      "BiasNanos",
                                                      "BiasUncertaintyNanos",
                                                      "DriftNanosPerSecond",
                                                      "DriftUncertaintyNanosPerSecond",
                                                      "HardwareClockDiscontinuityCount",
                                                      "Svid",
                                                      "TimeOffsetNanos",
                                                      "State",
                                                      "ReceivedSvTimeNanos",
                                                      "ReceivedSvTimeUncertaintyNanos",
                                                      "Cn0DbHz",
                                                      "PseudorangeRateMetersPerSecond",
                                                      "PseudorangeRateUncertaintyMetersPerSecond",
                                                      "AccumulatedDeltaRangeState",
                                                      "AccumulatedDeltaRangeMeters",
                                                      "AccumulatedDeltaRangeUncertaintyMeters",
                                                      "CarrierFrequencyHz",
                                                      "CarrierCycles",
                                                      "CarrierPhase",
                                                      "CarrierPhaseUncertainty",
                                                      "MultipathIndicator",
                                                      "SnrInDb",
                                                      "ConstellationType",
                                                      "AgcDb",
                                                      "BasebandCn0DbHz",
                                                      "FullInterSignalBiasNanos",
                                                      "FullInterSignalBiasUncertaintyNanos",
                                                      "SatelliteInterSignalBiasNanos",
                                                      "SatelliteInterSignalBiasUncertaintyNanos",
                                                      "CodeType",
                                                      "ChipsetElapsedRealtimeNanos"};

    for (const auto& columnName : requiredHeaderColumns) {
        if (columnNameIdMapping.find(columnName) == columnNameIdMapping.end()) {
            ALOGE("Missing column %s in header.", columnName.c_str());
            return false;
        }
    }

    return true;
}

}  // namespace common
}  // namespace gnss
}  // namespace hardware
}  // namespace android
