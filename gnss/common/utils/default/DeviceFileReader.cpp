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
#include "DeviceFileReader.h"

namespace android {
namespace hardware {
namespace gnss {
namespace common {

void DeviceFileReader::getDataFromDeviceFile(const std::string& command, int mMinIntervalMs) {
    char inputBuffer[INPUT_BUFFER_SIZE];
    std::string deviceFilePath = "";
    if (command == CMD_GET_LOCATION) {
        deviceFilePath = ReplayUtils::getFixedLocationPath();
    } else if (command == CMD_GET_RAWMEASUREMENT) {
        deviceFilePath = ReplayUtils::getGnssPath();
    } else {
        // Invalid command
        return;
    }

    int gnss_fd, epoll_fd;
    if ((gnss_fd = open(deviceFilePath.c_str(), O_RDWR | O_NONBLOCK)) == -1) {
        return;
    }
    if (write(gnss_fd, command.c_str(), command.size()) <= 0) {
        close(gnss_fd);
        return;
    }

    // Create an epoll instance.
    if ((epoll_fd = epoll_create1(EPOLL_CLOEXEC)) < 0) {
        close(gnss_fd);
        return;
    }

    // Add file descriptor to epoll instance.
    struct epoll_event ev, events[1];
    memset(&ev, 0, sizeof(ev));
    ev.data.fd = gnss_fd;
    ev.events = EPOLLIN;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, gnss_fd, &ev) == -1) {
        close(gnss_fd);
        close(epoll_fd);
        return;
    }

    // Wait for device file event.
    if (epoll_wait(epoll_fd, events, 1, mMinIntervalMs) == -1) {
        close(gnss_fd);
        close(epoll_fd);
        return;
    }

    // Handle event and write data to string buffer.
    int bytes_read = -1;
    std::string inputStr = "";
    while (true) {
        memset(inputBuffer, 0, INPUT_BUFFER_SIZE);
        bytes_read = read(gnss_fd, &inputBuffer, INPUT_BUFFER_SIZE);
        if (bytes_read <= 0) {
            break;
        }
        s_buffer_ += std::string(inputBuffer, bytes_read);
    }
    close(gnss_fd);
    close(epoll_fd);

    // Trim end of file mark(\n\n\n\n).
    auto pos = s_buffer_.find("\n\n\n\n");
    if (pos != std::string::npos) {
        inputStr = s_buffer_.substr(0, pos);
        s_buffer_ = s_buffer_.substr(pos + 4);
    } else {
        return;
    }

    // Cache the injected data.
    if (command == CMD_GET_LOCATION) {
        // TODO validate data
        data_[CMD_GET_LOCATION] = inputStr;
    } else if (command == CMD_GET_RAWMEASUREMENT) {
        if (ReplayUtils::isGnssRawMeasurement(inputStr)) {
            data_[CMD_GET_RAWMEASUREMENT] = inputStr;
        }
    }
}

std::string DeviceFileReader::getLocationData() {
    std::unique_lock<std::mutex> lock(mMutex);
    getDataFromDeviceFile(CMD_GET_LOCATION, 20);
    return data_[CMD_GET_LOCATION];
}

std::string DeviceFileReader::getGnssRawMeasurementData() {
    std::unique_lock<std::mutex> lock(mMutex);
    getDataFromDeviceFile(CMD_GET_RAWMEASUREMENT, 20);
    return data_[CMD_GET_RAWMEASUREMENT];
}

DeviceFileReader::DeviceFileReader() {}

DeviceFileReader::~DeviceFileReader() {}

}  // namespace common
}  // namespace gnss
}  // namespace hardware
}  // namespace android
