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

#ifndef android_hardware_gnss_common_default_GnssRawMeasurementParser_H_
#define android_hardware_gnss_common_default_GnssRawMeasurementParser_H_

#include <aidl/android/hardware/gnss/BnGnss.h>
#include <log/log.h>
#include <utils/SystemClock.h>
#include <string>
#include <unordered_map>

#include "Constants.h"
#include "ParseUtils.h"

namespace android {
namespace hardware {
namespace gnss {
namespace common {

struct GnssRawMeasurementParser {
    static std::unique_ptr<aidl::android::hardware::gnss::GnssData> getMeasurementFromStrs(
            std::string& rawMeasurementStr);
    static int getClockFlags(const std::vector<std::string>& rawMeasurementRecordValues,
                             const std::unordered_map<std::string, int>& columnNameIdMapping);
    static int getElapsedRealtimeFlags(
            const std::vector<std::string>& rawMeasurementRecordValues,
            const std::unordered_map<std::string, int>& columnNameIdMapping);
    static int getRawMeasurementFlags(
            const std::vector<std::string>& rawMeasurementRecordValues,
            const std::unordered_map<std::string, int>& columnNameIdMapping);
    static std::unordered_map<std::string, int> getColumnIdNameMappingFromHeader(
            const std::string& header);
    static aidl::android::hardware::gnss::GnssConstellationType getGnssConstellationType(
            int constellationType);
};

}  // namespace common
}  // namespace gnss
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_gnss_common_default_GnssRawMeasurementParser_H_