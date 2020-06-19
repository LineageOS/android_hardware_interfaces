/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <libnetdevice/ethtool.h>

#include "ifreqs.h"

#include <linux/ethtool.h>

namespace android::netdevice::ethtool {

std::optional<uint32_t> getValue(const std::string& ifname, uint32_t command) {
    struct ethtool_value valueop = {};
    valueop.cmd = command;

    auto ifr = ifreqs::fromName(ifname);
    ifr.ifr_data = &valueop;

    if (!ifreqs::send(SIOCETHTOOL, ifr)) return std::nullopt;
    return valueop.data;
}

bool setValue(const std::string& ifname, uint32_t command, uint32_t value) {
    struct ethtool_value valueop = {};
    valueop.cmd = command;
    valueop.data = value;

    auto ifr = ifreqs::fromName(ifname);
    ifr.ifr_data = &valueop;

    return ifreqs::send(SIOCETHTOOL, ifr);
}

}  // namespace android::netdevice::ethtool
