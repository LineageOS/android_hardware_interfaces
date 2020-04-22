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

#include "CanController.h"

#include "CanBusNative.h"
#include "CanBusSlcan.h"
#include "CanBusVirtual.h"

#include <android-base/logging.h>
#include <android/hidl/manager/1.2/IServiceManager.h>

#include <automotive/filesystem>
#include <fstream>
#include <regex>

namespace android::hardware::automotive::can::V1_0::implementation {

using IfId = ICanController::BusConfig::InterfaceId;
using IfIdDisc = ICanController::BusConfig::InterfaceId::hidl_discriminator;
namespace fs = android::hardware::automotive::filesystem;

namespace fsErrors {
static const std::error_code ok;
static const std::error_code eperm(EPERM, std::generic_category());
static const std::error_code enoent(ENOENT, std::generic_category());
static const std::error_code eacces(EACCES, std::generic_category());
}  // namespace fsErrors

/* In the /sys/devices tree, there are files called "serial", which contain the serial numbers
 * for various devices. The exact location inside of this directory is dependent upon the
 * hardware we are running on, so we have to start from /sys/devices and work our way down. */
static const fs::path kDevPath("/sys/devices/");
static const std::regex kTtyRe("^tty[A-Z]+[0-9]+$");
static constexpr auto kOpts = ~(fs::directory_options::follow_directory_symlink |
                                fs::directory_options::skip_permission_denied);

/**
 * A helper object to associate the interface name and type of a USB to CAN adapter.
 */
struct UsbCanIface {
    ICanController::InterfaceType iftype;
    std::string ifaceName;
};

Return<void> CanController::getSupportedInterfaceTypes(getSupportedInterfaceTypes_cb _hidl_cb) {
    _hidl_cb({ICanController::InterfaceType::VIRTUAL, ICanController::InterfaceType::SOCKETCAN,
              ICanController::InterfaceType::SLCAN});
    return {};
}

static bool isValidName(const std::string& name) {
    static const std::regex nameRE("^[a-zA-Z0-9_]{1,32}$");
    return std::regex_match(name, nameRE);
}

/**
 * Given a UsbCanIface object, get the ifaceName given the serialPath.
 *
 * \param serialPath - Absolute path to a "serial" file for a given device in /sys.
 * \return A populated UsbCanIface. On failure, nullopt is returned.
 */
static std::optional<UsbCanIface> getIfaceName(fs::path serialPath) {
    std::error_code fsStatus;
    // Since the path is to a file called "serial", we need to search its parent directory.
    fs::recursive_directory_iterator fsItr(serialPath.parent_path(), kOpts, fsStatus);
    if (fsStatus != fsErrors::ok) {
        LOG(ERROR) << "Failed to open " << serialPath.parent_path();
        return std::nullopt;
    }

    for (; fsStatus == fsErrors::ok && fsItr != fs::recursive_directory_iterator();
         fsItr.increment(fsStatus)) {
        /* We want either a directory called "net" or a directory that looks like tty<something>, so
         * skip files. */
        bool isDir = fsItr->is_directory(fsStatus);
        if (fsStatus != fsErrors::ok || !isDir) continue;

        /* path() returns an iterator that steps through directories from / to the leaf.
         * end() returns one past the leaf of the path, but we want the leaf. Decrementing the
         * path gives us a pointer to the leaf, which we then dereference.*/
        std::string currentDir = *(--(fsItr->path().end()));
        if (currentDir == "net") {
            /* This device is a SocketCAN device. The iface name is the only directory under
             * net/. Multiple directories under net/ is an error.*/
            fs::directory_iterator netItr(fsItr->path(), kOpts, fsStatus);
            if (fsStatus != fsErrors::ok) {
                LOG(ERROR) << "Failed to open " << fsItr->path() << " to get net name!";
                return std::nullopt;
            }

            // Get the leaf of the path. This is the interface name, assuming it's the only leaf.
            std::string netName = *(--(netItr->path().end()));

            // Check if there is more than one item in net/
            netItr.increment(fsStatus);
            if (fsStatus != fsErrors::ok) {
                // It's possible we have a valid net name, but this is most likely an error.
                LOG(ERROR) << "Failed to verify " << fsItr->path() << " has valid net name!";
                return std::nullopt;
            }
            if (netItr != fs::directory_iterator()) {
                // There should never be more than one name under net/
                LOG(ERROR) << "Found more than one net name in " << fsItr->path() << "!";
                return std::nullopt;
            }
            return {{ICanController::InterfaceType::SOCKETCAN, netName}};
        } else if (std::regex_match(currentDir, kTtyRe)) {
            // This device is a USB serial device, and currentDir is the tty name.
            return {{ICanController::InterfaceType::SLCAN, "/dev/" + currentDir}};
        }
    }

    // check if the loop above exited due to a c++fs error.
    if (fsStatus != fsErrors::ok) {
        LOG(ERROR) << "Failed search filesystem: " << fsStatus;
    }
    return std::nullopt;
}

/**
 * A helper function to read the serial number from a "serial" file in /sys/devices/
 *
 * \param serialnoPath - path to the file to read.
 * \return the serial number, or nullopt on failure.
 */
static std::optional<std::string> readSerialNo(const std::string& serialnoPath) {
    std::ifstream serialnoStream(serialnoPath);
    std::string serialno;
    if (!serialnoStream.good()) {
        LOG(ERROR) << "Failed to read serial number from " << serialnoPath;
        return std::nullopt;
    }
    std::getline(serialnoStream, serialno);
    return serialno;
}

/**
 * Searches for USB devices found in /sys/devices/, and attempts to find a device matching the
 * provided list of serial numbers.
 *
 * \param configSerialnos - a list of serial number (suffixes) from the HAL config.
 * \param iftype - the type of the interface to be located.
 * \return a matching USB device. On failure, std::nullopt is returned.
 */
static std::optional<UsbCanIface> findUsbDevice(const hidl_vec<hidl_string>& configSerialnos) {
    std::error_code fsStatus;
    fs::recursive_directory_iterator fsItr(kDevPath, kOpts, fsStatus);
    if (fsStatus != fsErrors::ok) {
        LOG(ERROR) << "Failed to open " << kDevPath;
        return std::nullopt;
    }

    for (; fsStatus == fsErrors::ok && fsItr != fs::recursive_directory_iterator();
         fsItr.increment(fsStatus)) {
        // We want to find a file called "serial", which is in a directory somewhere. Skip files.
        bool isDir = fsItr->is_directory(fsStatus);
        if (fsStatus != fsErrors::ok) {
            LOG(ERROR) << "Failed check if " << fsStatus;
            return std::nullopt;
        }
        if (!isDir) continue;

        auto serialnoPath = fsItr->path() / "serial";
        bool isReg = fs::is_regular_file(serialnoPath, fsStatus);

        /* Make sure we have permissions to this directory, ignore enoent, since the file
         * "serial" may not exist, which is ok. */
        if (fsStatus == fsErrors::eperm || fsStatus == fsErrors::eacces) {
            /* This means we  don't have access to this directory. If we recurse into it, this
             * will cause the iterator to loose its state and we'll crash. */
            fsItr.disable_recursion_pending();
            continue;
        }
        if (fsStatus == fsErrors::enoent) continue;
        if (fsStatus != fsErrors::ok) {
            LOG(WARNING) << "An unexpected error occurred while checking for serialno: "
                         << fsStatus;
            continue;
        }
        if (!isReg) continue;

        // we found a serial number
        auto serialno = readSerialNo(serialnoPath);
        if (!serialno.has_value()) continue;

        // see if the serial number exists in the config
        for (auto&& cfgSn : configSerialnos) {
            if (serialno->ends_with(std::string(cfgSn))) {
                auto ifaceInfo = getIfaceName(serialnoPath);
                if (!ifaceInfo.has_value()) break;
                return ifaceInfo;
            }
        }
    }
    if (fsStatus != fsErrors::ok) {
        LOG(ERROR) << "Error searching filesystem: " << fsStatus;
        return std::nullopt;
    }
    return std::nullopt;
}

Return<ICanController::Result> CanController::upInterface(const ICanController::BusConfig& config) {
    LOG(VERBOSE) << "Attempting to bring interface up: " << toString(config);

    std::lock_guard<std::mutex> lck(mCanBusesGuard);

    if (!isValidName(config.name)) {
        LOG(ERROR) << "Bus name " << config.name << " is invalid";
        return ICanController::Result::BAD_SERVICE_NAME;
    }

    if (mCanBuses.find(config.name) != mCanBuses.end()) {
        LOG(ERROR) << "Bus " << config.name << " is already up";
        return ICanController::Result::INVALID_STATE;
    }

    sp<CanBus> busService;

    // SocketCAN native type interface.
    if (config.interfaceId.getDiscriminator() == IfIdDisc::socketcan) {
        auto& socketcan = config.interfaceId.socketcan();
        std::string ifaceName;
        if (socketcan.getDiscriminator() == IfId::Socketcan::hidl_discriminator::serialno) {
            // Configure by serial number.
            auto selectedDevice = findUsbDevice(socketcan.serialno());
            // verify the returned device is the correct one
            if (!selectedDevice.has_value() ||
                selectedDevice->iftype != ICanController::InterfaceType::SOCKETCAN) {
                return ICanController::Result::BAD_INTERFACE_ID;
            }
            ifaceName = selectedDevice->ifaceName;
        } else {
            // configure by iface name.
            ifaceName = socketcan.ifname();
        }
        busService = new CanBusNative(ifaceName, config.bitrate);
    }
    // Virtual interface.
    else if (config.interfaceId.getDiscriminator() == IfIdDisc::virtualif) {
        busService = new CanBusVirtual(config.interfaceId.virtualif().ifname);
    }
    // SLCAN interface.
    else if (config.interfaceId.getDiscriminator() == IfIdDisc::slcan) {
        auto& slcan = config.interfaceId.slcan();
        std::string ttyName;
        if (slcan.getDiscriminator() == IfId::Slcan::hidl_discriminator::serialno) {
            // Configure by serial number.
            auto selectedDevice = findUsbDevice(slcan.serialno());
            if (!selectedDevice.has_value() ||
                selectedDevice->iftype != ICanController::InterfaceType::SLCAN) {
                return ICanController::Result::BAD_INTERFACE_ID;
            }
            ttyName = selectedDevice->ifaceName;
        } else {
            // Configure by tty name.
            ttyName = slcan.ttyname();
        }
        busService = new CanBusSlcan(ttyName, config.bitrate);
    } else {
        return ICanController::Result::NOT_SUPPORTED;
    }

    busService->setErrorCallback([this, name = config.name]() { downInterface(name); });

    const auto result = busService->up();
    if (result != ICanController::Result::OK) return result;

    if (busService->registerAsService(config.name) != OK) {
        LOG(ERROR) << "Failed to register ICanBus/" << config.name;
        if (!busService->down()) {
            LOG(WARNING) << "Failed to bring down CAN bus that failed to register";
        }
        return ICanController::Result::BAD_SERVICE_NAME;
    }

    mCanBuses[config.name] = busService;

    return ICanController::Result::OK;
}

static bool unregisterCanBusService(const hidl_string& name, sp<CanBus> busService) {
    auto manager = hidl::manager::V1_2::IServiceManager::getService();
    if (!manager) return false;
    const auto res = manager->tryUnregister(ICanBus::descriptor, name, busService);
    if (!res.isOk()) return false;
    return res;
}

Return<bool> CanController::downInterface(const hidl_string& name) {
    LOG(VERBOSE) << "Attempting to bring interface down: " << name;

    std::lock_guard<std::mutex> lck(mCanBusesGuard);

    auto busEntry = mCanBuses.extract(name);
    if (!busEntry) {
        LOG(WARNING) << "Interface " << name << " is not up";
        return false;
    }

    auto success = true;

    if (!unregisterCanBusService(name, busEntry.mapped())) {
        LOG(ERROR) << "Couldn't unregister " << name;
        // don't return yet, let's try to do best-effort cleanup
        success = false;
    }

    if (!busEntry.mapped()->down()) {
        LOG(ERROR) << "Couldn't bring " << name << " down";
        success = false;
    }

    return success;
}

}  // namespace android::hardware::automotive::can::V1_0::implementation
