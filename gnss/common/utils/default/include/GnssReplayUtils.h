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

#ifndef android_hardware_gnss_common_GnssReplayUtils_H_
#define android_hardware_gnss_common_GnssReplayUtils_H_

#include <cutils/properties.h>
#include <errno.h>
#include <fcntl.h>
#include <log/log.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <chrono>
#include <string>
#include <thread>

#include "Constants.h"

namespace android {
namespace hardware {
namespace gnss {
namespace common {

struct ReplayUtils {
    static std::string getGnssPath();

    static std::string getFixedLocationPath();

    static std::string getDataFromDeviceFile(const std::string& command, int mMinIntervalMs);

    static bool hasGnssDeviceFile();

    static bool hasFixedLocationDeviceFile();

    static bool isGnssRawMeasurement(const std::string& inputStr);

    static bool isNMEA(const std::string& inputStr);
};

}  // namespace common
}  // namespace gnss
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_gnss_common_GnssReplayUtils_H_
