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

#include "GnssReplayUtils.h"

#include <array>

namespace android {
namespace hardware {
namespace gnss {
namespace common {

std::string ReplayUtils::getGnssPath() {
    std::array<char, PROPERTY_VALUE_MAX> devname_value;

    devname_value.fill(0);
    if (property_get("debug.location.gnss.devname", devname_value.begin(), NULL) > 0) {
        return devname_value.begin();
    }

    devname_value.fill(0);
    if (property_get("vendor.ser.gnss-uart", devname_value.begin(), NULL) > 0) {
        return devname_value.begin();
    }

    return GNSS_PATH;
}

std::string ReplayUtils::getFixedLocationPath() {
    std::array<char, PROPERTY_VALUE_MAX> devname_value;

    devname_value.fill(0);
    if (property_get("debug.location.fixedlocation.devname", devname_value.begin(), NULL) > 0) {
        return devname_value.begin();
    }

    devname_value.fill(0);
    if (property_get("vendor.ser.gnss-uart", devname_value.begin(), NULL) > 0) {
        return devname_value.begin();
    }

    return FIXED_LOCATION_PATH;
}

bool ReplayUtils::hasGnssDeviceFile() {
    struct stat sb;
    return stat(getGnssPath().c_str(), &sb) != -1;
}

bool ReplayUtils::hasFixedLocationDeviceFile() {
    struct stat sb;
    return stat(getFixedLocationPath().c_str(), &sb) != -1;
}

bool ReplayUtils::isGnssRawMeasurement(const std::string& inputStr) {
    // TODO: add more logic check to by pass invalid data.
    return !inputStr.empty() && (inputStr.find("Raw") != std::string::npos);
}

bool ReplayUtils::isNMEA(const std::string& inputStr) {
    return !inputStr.empty() && (inputStr.find("$GPRMC,", 0) != std::string::npos ||
                                 inputStr.find("$GPRMA,", 0) != std::string::npos);
}

}  // namespace common
}  // namespace gnss
}  // namespace hardware
}  // namespace android
