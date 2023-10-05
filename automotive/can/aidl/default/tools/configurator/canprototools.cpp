/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <aidl/android/hardware/automotive/can/IndexedInterface.h>
#include <aidl/android/hardware/automotive/can/NativeInterface.h>
#include <aidl/android/hardware/automotive/can/SlcanInterface.h>
#include <aidl/android/hardware/automotive/can/VirtualInterface.h>

#include <android-base/logging.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

#include <fstream>

namespace android::hardware::automotive::can::config {

using ::aidl::android::hardware::automotive::can::BusConfig;
using ::aidl::android::hardware::automotive::can::IndexedInterface;
using ::aidl::android::hardware::automotive::can::InterfaceType;
using ::aidl::android::hardware::automotive::can::NativeInterface;
using ::aidl::android::hardware::automotive::can::Result;
using ::aidl::android::hardware::automotive::can::SlcanInterface;
using ::aidl::android::hardware::automotive::can::VirtualInterface;

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

/*
  parseConfigFile *used to* contain the body of parseConfigStream. However, it seems there's some
  sort of odd behavior with IstreamInputStream and/or TextFormat::Parse, which causes HW Address
  Sanitizer to flag a "tag-mismatch" in this function. Having the ifstream defined in a wrapper
  function seems to solve this problem. The exact cause of this problem is yet unknown, but probably
  lies somewhere in the protobuf implementation.
*/
static __attribute__((noinline)) std::optional<CanBusConfig> parseConfigStream(
        std::ifstream& cfg_stream) {
    static const std::array<std::string, 3> text_headers = {"buses", "#", "controller"};
    auto cfg_file_snippet = readString(cfg_stream, 10);

    if (!cfg_file_snippet.has_value()) {
        LOG(ERROR) << "Can't read config from stream (maybe failed to open file?)";
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
            LOG(ERROR) << "Parsing text format config failed";
            return std::nullopt;
        }
    } else if (!config.ParseFromIstream(&cfg_stream)) {
        LOG(ERROR) << "Parsing binary format config failed";
        return std::nullopt;
    }
    return config;
}

std::optional<CanBusConfig> parseConfigFile(const std::string& filepath) {
    std::ifstream cfg_stream(filepath);
    auto cfg_maybe = parseConfigStream(cfg_stream);
    if (!cfg_maybe.has_value()) {
        LOG(ERROR) << "Failed to parse " << filepath;
    }
    return cfg_maybe;
}

std::optional<BusConfig> fromPbBus(const Bus& pb_bus) {
    BusConfig bus_cfg = {};
    bus_cfg.name = pb_bus.name();

    switch (pb_bus.iface_type_case()) {
        case Bus::kNative: {
            const std::string ifname = pb_bus.native().ifname();
            const std::vector<std::string> serials = {pb_bus.native().serialno().begin(),
                                                      pb_bus.native().serialno().end()};
            if (ifname.empty() == serials.empty()) {
                LOG(ERROR) << "Invalid config: native type bus must have an iface name xor a "
                           << "serial number";
                return std::nullopt;
            }
            bus_cfg.bitrate = pb_bus.bitrate();
            NativeInterface nativeif = {};
            if (!ifname.empty())
                nativeif.interfaceId.set<NativeInterface::InterfaceId::Tag::ifname>(ifname);
            if (!serials.empty())
                nativeif.interfaceId.set<NativeInterface::InterfaceId::Tag::serialno>(serials);
            bus_cfg.interfaceId.set<BusConfig::InterfaceId::Tag::nativeif>(nativeif);
            break;
        }
        case Bus::kSlcan: {
            const std::string ttyname = pb_bus.slcan().ttyname();
            const std::vector<std::string> serials = {pb_bus.slcan().serialno().begin(),
                                                      pb_bus.slcan().serialno().end()};
            if (ttyname.empty() == serials.empty()) {
                LOG(ERROR) << "Invalid config: slcan type bus must have a tty name xor a serial "
                           << "number";
                return std::nullopt;
            }
            bus_cfg.bitrate = pb_bus.bitrate();
            SlcanInterface slcan = {};
            if (!ttyname.empty())
                slcan.interfaceId.set<SlcanInterface::InterfaceId::Tag::ttyname>(ttyname);
            if (!serials.empty())
                slcan.interfaceId.set<SlcanInterface::InterfaceId::Tag::serialno>(serials);
            bus_cfg.interfaceId.set<BusConfig::InterfaceId::Tag::slcan>(slcan);
            break;
        }
        case Bus::kVirtual: {
            // Theoretically, we could just create the next available vcan iface.
            const std::string ifname = pb_bus.virtual_().ifname();
            if (ifname.empty()) {
                LOG(ERROR) << "Invalid config: native type bus must have an iface name";
                return std::nullopt;
            }
            VirtualInterface virtualif = {};
            virtualif.ifname = ifname;
            bus_cfg.interfaceId.set<BusConfig::InterfaceId::Tag::virtualif>(virtualif);
            break;
        }
        case Bus::kIndexed: {
            const uint8_t index = pb_bus.indexed().index();
            if (index > UINT8_MAX) {
                LOG(ERROR) << "Interface index out of range: " << index;
                return std::nullopt;
            }
            IndexedInterface indexedif = {};
            indexedif.index = index;
            bus_cfg.interfaceId.set<BusConfig::InterfaceId::Tag::indexed>(indexedif);
            break;
        }
        default:
            LOG(ERROR) << "Invalid config: bad interface type for " << bus_cfg.name;
            return std::nullopt;
    }
    return bus_cfg;
}

std::optional<InterfaceType> getHalIftype(const Bus& pb_bus) {
    switch (pb_bus.iface_type_case()) {
        case Bus::kNative:
            return InterfaceType::NATIVE;
        case Bus::kSlcan:
            return InterfaceType::SLCAN;
        case Bus::kVirtual:
            return InterfaceType::VIRTUAL;
        case Bus::kIndexed:
            return InterfaceType::INDEXED;
        default:
            return std::nullopt;
    }
}

std::string resultStringFromStatus(const ndk::ScopedAStatus& status) {
    const auto res = static_cast<Result>(status.getServiceSpecificError());
    switch (res) {
        case Result::OK:
            return "OK";
        case Result::UNKNOWN_ERROR:
            return "UNKNOWN_ERROR";
        case Result::INVALID_STATE:
            return "INVALID_STATE";
        case Result::NOT_SUPPORTED:
            return "NOT_SUPPORTED";
        case Result::BAD_INTERFACE_ID:
            return "BAD_INTERFACE_ID";
        case Result::BAD_BITRATE:
            return "BAD_BITRATE";
        case Result::BAD_BUS_NAME:
            return "BAD_BUS_NAME";
        case Result::INTERFACE_DOWN:
            return "INTERFACE_DOWN";
        default:
            return "Invalid Result!";
    }
}

}  // namespace android::hardware::automotive::can::config
