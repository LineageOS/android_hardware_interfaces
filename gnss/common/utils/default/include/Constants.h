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

#ifndef android_hardware_gnss_common_Constants_H_
#define android_hardware_gnss_common_Constants_H_

#include <cstdint>

namespace android {
namespace hardware {
namespace gnss {
namespace common {

const float kMockHorizontalAccuracyMeters = 5;
const float kMockVerticalAccuracyMeters = 5;
const float kMockSpeedAccuracyMetersPerSecond = 1;
const float kMockBearingAccuracyDegrees = 90;
const int64_t kMockTimestamp = 1519930775453L;
const float kGpsL1FreqHz = 1575.42 * 1e6;
const float kGpsL5FreqHz = 1176.45 * 1e6;
const float kGloG1FreqHz = 1602.0 * 1e6;
const float kIrnssL5FreqHz = 1176.45 * 1e6;

// Location replay constants
constexpr char GNSS_PATH[] = "/dev/gnss0";
constexpr char FIXED_LOCATION_PATH[] = "/dev/gnss1";
constexpr int INPUT_BUFFER_SIZE = 256;
constexpr char CMD_GET_LOCATION[] = "CMD_GET_LOCATION";
constexpr char CMD_GET_RAWMEASUREMENT[] = "CMD_GET_RAWMEASUREMENT";
constexpr char LINE_SEPARATOR = '\n';
constexpr char COMMA_SEPARATOR = ',';
constexpr char GPGA_RECORD_TAG[] = "$GPGGA";
constexpr char GPRMC_RECORD_TAG[] = "$GPRMC";
constexpr double TIMESTAMP_EPSILON = 0.001;
constexpr int MIN_COL_NUM = 13;

}  // namespace common
}  // namespace gnss
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_gnss_common_Constants_H_
