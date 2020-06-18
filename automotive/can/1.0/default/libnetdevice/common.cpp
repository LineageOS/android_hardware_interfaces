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

namespace android::netdevice {

socketparams::Params socketparams::current = general;

unsigned int nametoindex(const std::string& ifname) {
    const auto ifidx = if_nametoindex(ifname.c_str());
    if (ifidx != 0) return ifidx;

    const auto err = errno;
    if (err != ENODEV) {
        LOG(ERROR) << "if_nametoindex(" << ifname << ") failed: " << err;
    }
    return 0;
}

std::string sanitize(std::string str) {
    str.erase(std::find(str.begin(), str.end(), '\0'), str.end());

    const auto isInvalid = [](char c) { return !isprint(c); };
    std::replace_if(str.begin(), str.end(), isInvalid, '?');

    return str;
}

uint16_t crc16(const nlbuf<uint8_t> data, uint16_t crc) {
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

}  // namespace android::netdevice
