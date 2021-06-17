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

namespace android {
namespace hardware {
namespace gnss {
namespace common {

const char* ReplayUtils::getGnssPath() {
    const char* gnss_dev_path = GNSS_PATH;
    char devname_value[PROPERTY_VALUE_MAX] = "";
    if (property_get("debug.location.gnss.devname", devname_value, NULL) > 0) {
        gnss_dev_path = devname_value;
    }
    return gnss_dev_path;
}

bool ReplayUtils::hasGnssDeviceFile() {
    struct stat sb;
    return stat(getGnssPath(), &sb) != -1;
}

bool ReplayUtils::isGnssRawMeasurement(const std::string& inputStr) {
    // TODO: add more logic check to by pass invalid data.
    return !inputStr.empty() && (inputStr.find("Raw") != std::string::npos);
}

bool ReplayUtils::isNMEA(const std::string& inputStr) {
    return !inputStr.empty() &&
           (inputStr.rfind("$GPRMC,", 0) == 0 || inputStr.rfind("$GPRMA,", 0) == 0);
}

std::string ReplayUtils::getDataFromDeviceFile(const std::string& command, int mMinIntervalMs) {
    char inputBuffer[INPUT_BUFFER_SIZE];
    int mGnssFd = open(getGnssPath(), O_RDWR | O_NONBLOCK);

    if (mGnssFd == -1) {
        return "";
    }

    int bytes_write = write(mGnssFd, command.c_str(), command.size());
    if (bytes_write <= 0) {
        return "";
    }

    struct epoll_event ev, events[1];
    ev.data.fd = mGnssFd;
    ev.events = EPOLLIN;
    int epoll_fd = epoll_create1(0);
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, mGnssFd, &ev);
    int bytes_read = -1;
    std::string inputStr = "";
    int epoll_ret = epoll_wait(epoll_fd, events, 1, mMinIntervalMs);

    if (epoll_ret == -1) {
        return "";
    }
    while (true) {
        memset(inputBuffer, 0, INPUT_BUFFER_SIZE);
        bytes_read = read(mGnssFd, &inputBuffer, INPUT_BUFFER_SIZE);
        if (bytes_read <= 0) {
            break;
        }
        inputStr += std::string(inputBuffer, bytes_read);
    }

    return inputStr;
}

}  // namespace common
}  // namespace gnss
}  // namespace hardware
}  // namespace android
