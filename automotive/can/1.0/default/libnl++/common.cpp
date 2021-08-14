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

#include "common.h"

#include <android-base/logging.h>

#include <net/if.h>

namespace android::nl {

unsigned int nametoindex(const std::string& ifname) {
    const auto ifidx = if_nametoindex(ifname.c_str());
    if (ifidx != 0) return ifidx;

    if (errno != ENODEV) {
        PLOG(ERROR) << "if_nametoindex(" << ifname << ") failed";
    }
    return 0;
}

std::string printableOnly(std::string str) {
    const auto isInvalid = [](char c) { return !isprint(c); };
    std::replace_if(str.begin(), str.end(), isInvalid, '?');

    return str;
}

uint16_t crc16(const Buffer<uint8_t> data, uint16_t crc) {
    for (const auto byte : data.getRaw()) {
        crc ^= byte;
        for (unsigned i = 0; i < 8; i++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

}  // namespace android::nl
