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

#ifndef android_hardware_gnss_common_vts_Utils_H_
#define android_hardware_gnss_common_vts_Utils_H_

#include <android/hardware/gnss/1.0/IGnss.h>
#include <android/hardware/gnss/measurement_corrections/1.0/IMeasurementCorrections.h>
#include <android/hardware/gnss/measurement_corrections/1.1/IMeasurementCorrections.h>

using GnssLocation = ::android::hardware::gnss::V1_0::GnssLocation;
using namespace android::hardware::gnss::measurement_corrections::V1_0;

using MeasurementCorrections_1_0 =
        android::hardware::gnss::measurement_corrections::V1_0::MeasurementCorrections;
using MeasurementCorrections_1_1 =
        android::hardware::gnss::measurement_corrections::V1_1::MeasurementCorrections;

namespace android {
namespace hardware {
namespace gnss {
namespace common {

struct Utils {
    static void checkLocation(const GnssLocation& location, bool check_speed,
                              bool check_more_accuracies);
    static const MeasurementCorrections_1_0 getMockMeasurementCorrections();
    static const MeasurementCorrections_1_1 getMockMeasurementCorrections_1_1();
};

}  // namespace common
}  // namespace gnss
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_gnss_common_vts_Utils_H_
