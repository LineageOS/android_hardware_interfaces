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

#pragma once

#include <android-base/unique_fd.h>

#include <string>

namespace android {
namespace netdevice {
namespace can {

/**
 * Opens and binds SocketCAN socket.
 *
 * @param ifname Interface to open a socket against
 * @return Socket's FD or -1 in case of failure
 */
base::unique_fd socket(const std::string& ifname);

/**
 * Sets CAN interface bitrate.
 */
bool setBitrate(std::string ifname, uint32_t bitrate);

}  // namespace can
}  // namespace netdevice
}  // namespace android
