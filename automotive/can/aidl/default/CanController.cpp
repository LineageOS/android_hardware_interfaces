/*
 * Copyright 2022, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
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

#include <android-base/format.h>
#include <android-base/logging.h>

#include <automotive/filesystem>
#include <fstream>
#include <regex>

namespace aidl::android::hardware::automotive::can {

namespace fs = ::android::hardware::automotive::filesystem;

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

constexpr auto ok = &ndk::ScopedAStatus::ok;

/**
 * A helper object to associate the interface name and type of a USB to CAN adapter.
 */
struct UsbCanIface {
    InterfaceType iftype;
    std::string ifaceName;
};

static bool isValidName(const std::string& name) {
    static const std::regex nameRE("^[a-zA-Z0-9_]{1,32}$");
    return std::regex_match(name, nameRE);
}

/**
 * Given a path, get the last element from it.
 *
 * \param itrPath - the path we want the last element of
 * \return - the last element in the path (in string form).
 */
static std::string getLeaf(const fs::path& itrPath) {
    /* end() returns an iterator one past the leaf of the path, so we've overshot
    decrement (--) to go back one to the leaf
    dereference and now we have our leaf. */
    return *(--(itrPath.end()));
}

static ndk::ScopedAStatus resultToStatus(Result res, const std::string& msg = "") {
    if (msg.empty()) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificError(static_cast<int>(res)));
    }
    return ndk::ScopedAStatus(
            AStatus_fromServiceSpecificErrorWithMessage(static_cast<int>(res), msg.c_str()));
}

/**
 * Given a UsbCanIface object, get the ifaceName given the serialPath.
 *
 * \param serialPath - Absolute path to a "serial" file for a given device in /sys.
 * \return A populated UsbCanIface. On failure, nullopt is returned.
 */
static std::optional<UsbCanIface> getIfaceName(const fs::path& serialPath) {
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

        std::string currentDir = getLeaf(fsItr->path());
        if (currentDir == "net") {
            /* This device is a SocketCAN device. The iface name is the only directory under
             * net/. Multiple directories under net/ is an error.*/
            fs::directory_iterator netItr(fsItr->path(), kOpts, fsStatus);
            if (fsStatus != fsErrors::ok) {
                LOG(ERROR) << "Failed to open " << fsItr->path() << " to get net name!";
                return std::nullopt;
            }

            // The leaf of our path should be the interface name.
            std::string netName = getLeaf(netItr->path());

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
            return {{InterfaceType::NATIVE, netName}};
        } else if (std::regex_match(currentDir, kTtyRe)) {
            // This device is a USB serial device, and currentDir is the tty name.
            return {{InterfaceType::SLCAN, "/dev/" + currentDir}};
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
static std::optional<UsbCanIface> findUsbDevice(const std::vector<std::string>& configSerialnos) {
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

ndk::ScopedAStatus CanController::getSupportedInterfaceTypes(
        std::vector<InterfaceType>* supportedTypes) {
    *supportedTypes = {InterfaceType::VIRTUAL, InterfaceType::NATIVE, InterfaceType::SLCAN};
    return ok();
}

ndk::ScopedAStatus CanController::getInterfaceName(const std::string& busName,
                                                   std::string* ifaceName) {
    *ifaceName = {};
    if (mBusesByName.find(busName) == mBusesByName.end()) {
        return resultToStatus(Result::BAD_BUS_NAME, fmt::format("{} doesn't exist", busName));
    }
    *ifaceName = std::string(mBusesByName[busName]->getIfaceName());
    return ok();
}

ndk::ScopedAStatus CanController::upBus(const BusConfig& config, std::string* ifaceName) {
    if (!isValidName(config.name)) {
        LOG(ERROR) << "Bus name " << config.name << " is invalid";
        return resultToStatus(Result::BAD_BUS_NAME,
                              fmt::format("{} is not a valid bus name", config.name));
    } else if (mBusesByName.find(config.name) != mBusesByName.end()) {
        LOG(ERROR) << "A bus named " << config.name << " already exists!";
        return resultToStatus(Result::INVALID_STATE,
                              fmt::format("A bus named {} already exists", config.name));
    }

    if (config.interfaceId.getTag() == BusConfig::InterfaceId::Tag::virtualif) {
        auto& virtualif = config.interfaceId.get<BusConfig::InterfaceId::Tag::virtualif>();
        mBusesByName[config.name] = std::make_unique<CanBusVirtual>(virtualif.ifname);
    }

    else if (config.interfaceId.getTag() == BusConfig::InterfaceId::Tag::nativeif) {
        auto& nativeif = config.interfaceId.get<BusConfig::InterfaceId::Tag::nativeif>();
        std::string ifaceName;
        if (nativeif.interfaceId.getTag() == NativeInterface::InterfaceId::Tag::serialno) {
            // Configure by serial number.
            auto selectedDevice = findUsbDevice(
                    nativeif.interfaceId.get<NativeInterface::InterfaceId::Tag::serialno>());
            // verify the returned device is the correct one
            if (!selectedDevice.has_value() || selectedDevice->iftype != InterfaceType::NATIVE) {
                return resultToStatus(
                        Result::BAD_INTERFACE_ID,
                        "Couldn't find a native socketcan device with the given serial number(s)");
            }
            ifaceName = selectedDevice->ifaceName;
        } else {
            // configure by iface name.
            ifaceName = nativeif.interfaceId.get<NativeInterface::InterfaceId::Tag::ifname>();
        }
        mBusesByName[config.name] = std::make_unique<CanBusNative>(ifaceName, config.bitrate);
    }

    else if (config.interfaceId.getTag() == BusConfig::InterfaceId::Tag::slcan) {
        auto& slcanif = config.interfaceId.get<BusConfig::InterfaceId::Tag::slcan>();
        std::string ttyName;
        if (slcanif.interfaceId.getTag() == SlcanInterface::InterfaceId::Tag::serialno) {
            // Configure by serial number.
            auto selectedDevice = findUsbDevice(
                    slcanif.interfaceId.get<SlcanInterface::InterfaceId::Tag::serialno>());
            if (!selectedDevice.has_value() || selectedDevice->iftype != InterfaceType::SLCAN) {
                return resultToStatus(
                        Result::BAD_INTERFACE_ID,
                        "Couldn't find a slcan device with the given serial number(s)");
            }
            ttyName = selectedDevice->ifaceName;
        } else {
            // Configure by tty name.
            ttyName = slcanif.interfaceId.get<SlcanInterface::InterfaceId::Tag::ttyname>();
        }
        mBusesByName[config.name] = std::make_unique<CanBusSlcan>(ttyName, config.bitrate);
    }

    else if (config.interfaceId.getTag() == BusConfig::InterfaceId::Tag::indexed) {
        return resultToStatus(Result::NOT_SUPPORTED,
                              "Indexed devices are not supported in this implementation");
    } else {
        // this shouldn't happen.
        return resultToStatus(Result::UNKNOWN_ERROR, "Unknown interface id type");
    }

    Result result = mBusesByName[config.name]->up();
    if (result != Result::OK) {
        // the bus failed to come up, don't leave a broken entry in the map.
        mBusesByName.erase(config.name);
        return resultToStatus(result, fmt::format("CanBus::up failed for {}", config.name));
    }

    *ifaceName = mBusesByName[config.name]->getIfaceName();
    return ok();
}

ndk::ScopedAStatus CanController::downBus(const std::string& busName) {
    if (mBusesByName.find(busName) == mBusesByName.end()) {
        return resultToStatus(
                Result::UNKNOWN_ERROR,
                fmt::format("Couldn't bring down {}, because it doesn't exist", busName));
    }
    Result result = mBusesByName[busName]->down();
    if (result != Result::OK) {
        return resultToStatus(result, fmt::format("Couldn't bring down {}!", busName));
    }
    mBusesByName.erase(busName);
    return ok();
}
}  // namespace aidl::android::hardware::automotive::can
