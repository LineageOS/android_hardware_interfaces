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

const float kMockLatitudeDegrees = 37.4219999;
const float kMockLongitudeDegrees = -122.0840575;
const float kMockAltitudeMeters = 1.60062531;
const float kMockSpeedMetersPerSec = 0;
const float kMockBearingDegrees = 0;
const float kMockHorizontalAccuracyMeters = 5;
const float kMockVerticalAccuracyMeters = 5;
const float kMockSpeedAccuracyMetersPerSecond = 1;
const float kMockBearingAccuracyDegrees = 90;
const int64_t kMockTimestamp = 1519930775453L;

}  // namespace common
}  // namespace gnss
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_gnss_common_Constants_H_
