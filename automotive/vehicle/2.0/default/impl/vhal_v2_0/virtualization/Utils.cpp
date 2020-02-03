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

#include "Utils.h"

#include <cutils/properties.h>

#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <sstream>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {
namespace impl {

std::string VsockServerInfo::toUri() {
    std::stringstream uri_stream;
    uri_stream << "vsock:" << serverCid << ":" << serverPort;
    return uri_stream.str();
}

static std::optional<unsigned> parseUnsignedIntFromString(const char* optarg, const char* name) {
    auto v = strtoul(optarg, nullptr, 0);
    if (((v == ULONG_MAX) && (errno == ERANGE)) || (v > UINT_MAX)) {
        LOG(WARNING) << name << " value is out of range: " << optarg;
    } else if (v != 0) {
        return v;
    } else {
        LOG(WARNING) << name << " value is invalid or missing: " << optarg;
    }

    return std::nullopt;
}

static std::optional<unsigned> getNumberFromProperty(const char* key) {
    auto value = property_get_int64(key, -1);
    if ((value <= 0) || (value > UINT_MAX)) {
        LOG(WARNING) << key << " is missing or out of bounds";
        return std::nullopt;
    }

    return static_cast<unsigned int>(value);
};

std::optional<VsockServerInfo> VsockServerInfo::fromCommandLine(int argc, char* argv[]) {
    std::optional<unsigned int> cid;
    std::optional<unsigned int> port;

    // unique values to identify the options
    constexpr int OPT_VHAL_SERVER_CID = 1001;
    constexpr int OPT_VHAL_SERVER_PORT_NUMBER = 1002;

    struct option longOptions[] = {
            {"server_cid", 1, 0, OPT_VHAL_SERVER_CID},
            {"server_port", 1, 0, OPT_VHAL_SERVER_PORT_NUMBER},
            {},
    };

    int optValue;
    while ((optValue = getopt_long_only(argc, argv, ":", longOptions, 0)) != -1) {
        switch (optValue) {
            case OPT_VHAL_SERVER_CID:
                cid = parseUnsignedIntFromString(optarg, "cid");
                break;
            case OPT_VHAL_SERVER_PORT_NUMBER:
                port = parseUnsignedIntFromString(optarg, "port");
                break;
            default:
                // ignore other options
                break;
        }
    }

    if (cid && port) {
        return VsockServerInfo{*cid, *port};
    }
    return std::nullopt;
}

std::optional<VsockServerInfo> VsockServerInfo::fromRoPropertyStore() {
    constexpr const char* VHAL_SERVER_CID_PROPERTY_KEY = "ro.vendor.vehiclehal.server.cid";
    constexpr const char* VHAL_SERVER_PORT_PROPERTY_KEY = "ro.vendor.vehiclehal.server.port";

    const auto cid = getNumberFromProperty(VHAL_SERVER_CID_PROPERTY_KEY);
    const auto port = getNumberFromProperty(VHAL_SERVER_PORT_PROPERTY_KEY);

    if (cid && port) {
        return VsockServerInfo{*cid, *port};
    }
    return std::nullopt;
}

}  // namespace impl
}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
