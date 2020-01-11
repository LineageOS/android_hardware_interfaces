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

#include <android-base/logging.h>
#include <android/hardware/automotive/can/1.0/ICanController.h>
#include <android/hidl/manager/1.2/IServiceManager.h>
#include <hidl-utils/hidl-utils.h>

#include <iostream>
#include <string>

namespace android::hardware::automotive::can {

using ICanController = V1_0::ICanController;

static void usage() {
    std::cerr << "CAN bus HAL Control tool" << std::endl;
    std::cerr << std::endl << "usage:" << std::endl << std::endl;
    std::cerr << "canhalctrl up <bus name> <type> <interface> [bitrate]" << std::endl;
    std::cerr << "where:" << std::endl;
    std::cerr << " bus name - name under which ICanBus will be published" << std::endl;
    std::cerr << " type - one of: virtual, socketcan, slcan, indexed" << std::endl;
    std::cerr << " interface - hardware identifier (like can0, vcan0, /dev/ttyUSB0)" << std::endl;
    std::cerr << " bitrate - such as 100000, 125000, 250000, 500000" << std::endl;
    std::cerr << std::endl;
    std::cerr << "canhalctrl down <bus name>" << std::endl;
    std::cerr << "where:" << std::endl;
    std::cerr << " bus name - name under which ICanBus will be published" << std::endl;
}

static hidl_vec<hidl_string> getControlServices() {
    auto manager = hidl::manager::V1_2::IServiceManager::getService();
    hidl_vec<hidl_string> services;
    manager->listManifestByInterface(ICanController::descriptor, hidl_utils::fill(&services));
    if (services.size() == 0) {
        std::cerr << "No ICanController services registered (missing privileges?)" << std::endl;
        exit(-1);
    }
    return services;
}

static bool isSupported(sp<ICanController> ctrl, ICanController::InterfaceType iftype) {
    hidl_vec<ICanController::InterfaceType> supported;
    if (!ctrl->getSupportedInterfaceTypes(hidl_utils::fill(&supported)).isOk()) return false;
    return supported.contains(iftype);
}

static int up(const std::string& busName, ICanController::InterfaceType type,
              const std::string& interface, uint32_t bitrate) {
    bool anySupported = false;
    for (auto&& service : getControlServices()) {
        auto ctrl = ICanController::getService(service);
        if (ctrl == nullptr) {
            std::cerr << "Couldn't open ICanController/" << service;
            continue;
        }

        if (!isSupported(ctrl, type)) continue;
        anySupported = true;

        ICanController::BusConfiguration config = {};
        config.name = busName;
        config.iftype = type;
        config.bitrate = bitrate;

        if (type == ICanController::InterfaceType::INDEXED) {
            config.interfaceId.index(std::stol(interface));
        } else {
            config.interfaceId.address(interface);
        }

        const auto upresult = ctrl->upInterface(config);
        if (upresult == ICanController::Result::OK) return 0;
        std::cerr << "Failed to bring interface up: " << toString(upresult) << std::endl;
        // Let's continue the loop to try other controllers.
    }

    if (!anySupported) {
        std::cerr << "No controller supports " << toString(type) << std::endl;
    }
    return -1;
}

static int down(const std::string& busName) {
    for (auto&& service : getControlServices()) {
        auto ctrl = ICanController::getService(service);
        if (ctrl == nullptr) continue;

        if (ctrl->downInterface(busName)) return 0;
    }

    std::cerr << "Failed to bring interface " << busName << " down (maybe it's down already?)"
              << std::endl;
    return -1;
}

static std::optional<ICanController::InterfaceType> parseInterfaceType(const std::string& str) {
    if (str == "virtual") return ICanController::InterfaceType::VIRTUAL;
    if (str == "socketcan") return ICanController::InterfaceType::SOCKETCAN;
    if (str == "slcan") return ICanController::InterfaceType::SLCAN;
    if (str == "indexed") return ICanController::InterfaceType::INDEXED;
    return std::nullopt;
}

static int main(int argc, char* argv[]) {
    base::SetDefaultTag("CanHalControl");
    base::SetMinimumLogSeverity(android::base::VERBOSE);

    if (argc == 0) {
        usage();
        return 0;
    }

    std::string cmd(argv[0]);
    argv++;
    argc--;

    if (cmd == "up") {
        if (argc < 3 || argc > 4) {
            std::cerr << "Invalid number of arguments to up command: " << argc << std::endl;
            usage();
            return -1;
        }

        const std::string busName(argv[0]);
        const std::string typeStr(argv[1]);
        const std::string interface(argv[2]);

        const auto type = parseInterfaceType(typeStr);
        if (!type) {
            std::cerr << "Invalid interface type: " << typeStr << std::endl;
            usage();
            return -1;
        }

        long long bitrate = 0;
        if (argc == 4) {
            bitrate = std::stoll(argv[3]);
        }

        return up(busName, *type, interface, bitrate);
    } else if (cmd == "down") {
        if (argc != 1) {
            std::cerr << "Invalid number of arguments to down command: " << argc << std::endl;
            usage();
            return -1;
        }

        return down(argv[0]);
    } else {
        std::cerr << "Invalid command: " << cmd << std::endl;
        usage();
        return -1;
    }
}

}  // namespace android::hardware::automotive::can

int main(int argc, char* argv[]) {
    if (argc < 1) return -1;
    return ::android::hardware::automotive::can::main(--argc, ++argv);
}
