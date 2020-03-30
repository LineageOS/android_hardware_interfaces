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
#include "canprototools.h"

#include <android-base/logging.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <hidl/HidlTransportSupport.h>
#include <libcanhaltools/libcanhaltools.h>

#include <fstream>

namespace android::hardware::automotive::can::config {

using ICanController = V1_0::ICanController;

/**
 * Helper function for parseConfigFile. readString is used to get the fist n characters (n) from an
 * istream object (s) and return it as a string object.
 *
 * \param s istream of the file you intend to read.
 * \param n streamsize object of the number of characters you'd like.
 * \return optional string containing up to n characters from the stream(s) you provided.
 */
static std::optional<std::string> readString(std::istream& s, std::streamsize n) {
    char buff[n];
    auto got = s.read(buff, n).gcount();
    if (!s.good() && !s.eof()) return std::nullopt;
    return std::string(buff, 0, std::min(n, got));
}

std::optional<CanBusConfig> parseConfigFile(const std::string& filepath) {
    std::ifstream cfg_stream(filepath);

    // text headers that would be present in a plaintext proto config file.
    static const std::array<std::string, 3> text_headers = {"buses", "#", "controller"};
    auto cfg_file_snippet = readString(cfg_stream, 10);

    if (!cfg_file_snippet.has_value()) {
        LOG(ERROR) << "Can't open " << filepath << " for reading";
        return std::nullopt;
    }
    cfg_stream.seekg(0);

    // check if any of the textHeaders are at the start of the config file.
    bool text_format = false;
    for (auto const& header : text_headers) {
        if (cfg_file_snippet->compare(0, header.length(), header) == 0) {
            text_format = true;
            break;
        }
    }

    CanBusConfig config;
    if (text_format) {
        google::protobuf::io::IstreamInputStream pb_stream(&cfg_stream);
        if (!google::protobuf::TextFormat::Parse(&pb_stream, &config)) {
            LOG(ERROR) << "Failed to parse (text format) " << filepath;
            return std::nullopt;
        }
    } else if (!config.ParseFromIstream(&cfg_stream)) {
        LOG(ERROR) << "Failed to parse (binary format) " << filepath;
        return std::nullopt;
    }
    return config;
}

std::optional<ICanController::BusConfig> fromPbBus(const Bus& pb_bus) {
    ICanController::BusConfig bus_cfg = {};
    bus_cfg.name = pb_bus.name();

    switch (pb_bus.iface_type_case()) {
        case Bus::kNative: {
            const auto ifname = pb_bus.native().ifname();
            const auto serialno = pb_bus.native().serialno();
            if (ifname.empty() == serialno.empty()) {
                LOG(ERROR) << "Invalid config: native type bus must have an iface name xor a "
                           << "serial number";
                return std::nullopt;
            }
            bus_cfg.bitrate = pb_bus.bitrate();
            ICanController::BusConfig::InterfaceId::Socketcan socketcan = {};
            if (!ifname.empty()) socketcan.ifname(ifname);
            if (!serialno.empty()) socketcan.serialno({serialno.begin(), serialno.end()});
            bus_cfg.interfaceId.socketcan(socketcan);
            break;
        }
        case Bus::kSlcan: {
            const auto ttyname = pb_bus.slcan().ttyname();
            const auto serialno = pb_bus.slcan().serialno();
            if (ttyname.empty() == serialno.empty()) {
                LOG(ERROR) << "Invalid config: slcan type bus must have a tty name";
                return std::nullopt;
            }
            bus_cfg.bitrate = pb_bus.bitrate();
            ICanController::BusConfig::InterfaceId::Slcan slcan = {};
            if (!ttyname.empty()) slcan.ttyname(ttyname);
            if (!serialno.empty()) slcan.serialno({serialno.begin(), serialno.end()});
            bus_cfg.interfaceId.slcan(slcan);
            break;
        }
        case Bus::kVirtual: {
            // Theoretically, we could just create the next available vcan iface.
            const auto ifname = pb_bus.virtual_().ifname();
            if (ifname.empty()) {
                LOG(ERROR) << "Invalid config: native type bus must have an iface name";
                return std::nullopt;
            }
            bus_cfg.interfaceId.virtualif({ifname});
            break;
        }
        case Bus::kIndexed: {
            const auto index = pb_bus.indexed().index();
            if (index > UINT8_MAX) {
                LOG(ERROR) << "Interface index out of range: " << index;
                return std::nullopt;
            }
            bus_cfg.interfaceId.indexed({uint8_t(index)});
            break;
        }
        default:
            LOG(ERROR) << "Invalid config: bad interface type for " << bus_cfg.name;
            return std::nullopt;
    }
    return bus_cfg;
}

std::optional<ICanController::InterfaceType> getHalIftype(const Bus& pb_bus) {
    switch (pb_bus.iface_type_case()) {
        case Bus::kNative:
            return ICanController::InterfaceType::SOCKETCAN;
        case Bus::kSlcan:
            return ICanController::InterfaceType::SLCAN;
        case Bus::kVirtual:
            return ICanController::InterfaceType::VIRTUAL;
        case Bus::kIndexed:
            return ICanController::InterfaceType::INDEXED;
        default:
            return std::nullopt;
    }
}

}  // namespace android::hardware::automotive::can::config
