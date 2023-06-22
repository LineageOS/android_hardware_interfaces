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

#include "wifi_chip.h"

#include <android-base/logging.h>
#include <android-base/unique_fd.h>
#include <cutils/properties.h>
#include <fcntl.h>
#include <net/if.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

#include "aidl_return_util.h"
#include "aidl_struct_util.h"
#include "wifi_legacy_hal.h"
#include "wifi_status_util.h"

#define P2P_MGMT_DEVICE_PREFIX "p2p-dev-"

namespace {
using aidl::android::hardware::wifi::IfaceType;
using aidl::android::hardware::wifi::IWifiChip;
using CoexRestriction = aidl::android::hardware::wifi::IWifiChip::CoexRestriction;
using ChannelCategoryMask = aidl::android::hardware::wifi::IWifiChip::ChannelCategoryMask;
using android::base::unique_fd;

constexpr char kCpioMagic[] = "070701";
constexpr size_t kMaxBufferSizeBytes = 1024 * 1024 * 3;
constexpr uint32_t kMaxRingBufferFileAgeSeconds = 60 * 60 * 10;
constexpr uint32_t kMaxRingBufferFileNum = 20;
constexpr char kTombstoneFolderPath[] = "/data/vendor/tombstones/wifi/";
constexpr char kActiveWlanIfaceNameProperty[] = "wifi.active.interface";
constexpr char kNoActiveWlanIfaceNamePropertyValue[] = "";
constexpr unsigned kMaxWlanIfaces = 5;
constexpr char kApBridgeIfacePrefix[] = "ap_br_";

template <typename Iface>
void invalidateAndClear(std::vector<std::shared_ptr<Iface>>& ifaces, std::shared_ptr<Iface> iface) {
    iface->invalidate();
    ifaces.erase(std::remove(ifaces.begin(), ifaces.end(), iface), ifaces.end());
}

template <typename Iface>
void invalidateAndClearAll(std::vector<std::shared_ptr<Iface>>& ifaces) {
    for (const auto& iface : ifaces) {
        iface->invalidate();
    }
    ifaces.clear();
}

template <typename Iface>
std::vector<std::string> getNames(std::vector<std::shared_ptr<Iface>>& ifaces) {
    std::vector<std::string> names;
    for (const auto& iface : ifaces) {
        names.emplace_back(iface->getName());
    }
    return names;
}

template <typename Iface>
std::shared_ptr<Iface> findUsingName(std::vector<std::shared_ptr<Iface>>& ifaces,
                                     const std::string& name) {
    std::vector<std::string> names;
    for (const auto& iface : ifaces) {
        if (name == iface->getName()) {
            return iface;
        }
    }
    return nullptr;
}

std::string getWlanIfaceName(unsigned idx) {
    if (idx >= kMaxWlanIfaces) {
        CHECK(false) << "Requested interface beyond wlan" << kMaxWlanIfaces;
        return {};
    }

    std::array<char, PROPERTY_VALUE_MAX> buffer;
    if (idx == 0 || idx == 1) {
        const char* altPropName = (idx == 0) ? "wifi.interface" : "wifi.concurrent.interface";
        auto res = property_get(altPropName, buffer.data(), nullptr);
        if (res > 0) return buffer.data();
    }
    std::string propName = "wifi.interface." + std::to_string(idx);
    auto res = property_get(propName.c_str(), buffer.data(), nullptr);
    if (res > 0) return buffer.data();

    return "wlan" + std::to_string(idx);
}

// Returns the dedicated iface name if defined.
// Returns two ifaces in bridged mode.
std::vector<std::string> getPredefinedApIfaceNames(bool is_bridged) {
    std::vector<std::string> ifnames;
    std::array<char, PROPERTY_VALUE_MAX> buffer;
    buffer.fill(0);
    if (property_get("ro.vendor.wifi.sap.interface", buffer.data(), nullptr) == 0) {
        return ifnames;
    }
    ifnames.push_back(buffer.data());
    if (is_bridged) {
        buffer.fill(0);
        if (property_get("ro.vendor.wifi.sap.concurrent.iface", buffer.data(), nullptr) == 0) {
            return ifnames;
        }
        ifnames.push_back(buffer.data());
    }
    return ifnames;
}

std::string getPredefinedP2pIfaceName() {
    std::array<char, PROPERTY_VALUE_MAX> primaryIfaceName;
    char p2pParentIfname[100];
    std::string p2pDevIfName = "";
    std::array<char, PROPERTY_VALUE_MAX> buffer;
    property_get("wifi.direct.interface", buffer.data(), "p2p0");
    if (strncmp(buffer.data(), P2P_MGMT_DEVICE_PREFIX, strlen(P2P_MGMT_DEVICE_PREFIX)) == 0) {
        /* Get the p2p parent interface name from p2p device interface name set
         * in property */
        strlcpy(p2pParentIfname, buffer.data() + strlen(P2P_MGMT_DEVICE_PREFIX),
                strlen(buffer.data()) - strlen(P2P_MGMT_DEVICE_PREFIX));
        if (property_get(kActiveWlanIfaceNameProperty, primaryIfaceName.data(), nullptr) == 0) {
            return buffer.data();
        }
        /* Check if the parent interface derived from p2p device interface name
         * is active */
        if (strncmp(p2pParentIfname, primaryIfaceName.data(),
                    strlen(buffer.data()) - strlen(P2P_MGMT_DEVICE_PREFIX)) != 0) {
            /*
             * Update the predefined p2p device interface parent interface name
             * with current active wlan interface
             */
            p2pDevIfName += P2P_MGMT_DEVICE_PREFIX;
            p2pDevIfName += primaryIfaceName.data();
            LOG(INFO) << "update the p2p device interface name to " << p2pDevIfName.c_str();
            return p2pDevIfName;
        }
    }
    return buffer.data();
}

// Returns the dedicated iface name if one is defined.
std::string getPredefinedNanIfaceName() {
    std::array<char, PROPERTY_VALUE_MAX> buffer;
    if (property_get("wifi.aware.interface", buffer.data(), nullptr) == 0) {
        return {};
    }
    return buffer.data();
}

void setActiveWlanIfaceNameProperty(const std::string& ifname) {
    auto res = property_set(kActiveWlanIfaceNameProperty, ifname.data());
    if (res != 0) {
        PLOG(ERROR) << "Failed to set active wlan iface name property";
    }
}

// Delete files that meet either condition:
// 1. Older than a predefined time in the wifi tombstone dir.
// 2. Files in excess to a predefined amount, starting from the oldest ones
bool removeOldFilesInternal() {
    time_t now = time(0);
    const time_t delete_files_before = now - kMaxRingBufferFileAgeSeconds;
    std::unique_ptr<DIR, decltype(&closedir)> dir_dump(opendir(kTombstoneFolderPath), closedir);
    if (!dir_dump) {
        PLOG(ERROR) << "Failed to open directory";
        return false;
    }
    struct dirent* dp;
    bool success = true;
    std::list<std::pair<const time_t, std::string>> valid_files;
    while ((dp = readdir(dir_dump.get()))) {
        if (dp->d_type != DT_REG) {
            continue;
        }
        std::string cur_file_name(dp->d_name);
        struct stat cur_file_stat;
        std::string cur_file_path = kTombstoneFolderPath + cur_file_name;
        if (stat(cur_file_path.c_str(), &cur_file_stat) == -1) {
            PLOG(ERROR) << "Failed to get file stat for " << cur_file_path;
            success = false;
            continue;
        }
        const time_t cur_file_time = cur_file_stat.st_mtime;
        valid_files.push_back(std::pair<const time_t, std::string>(cur_file_time, cur_file_path));
    }
    valid_files.sort();  // sort the list of files by last modified time from
                         // small to big.
    uint32_t cur_file_count = valid_files.size();
    for (auto cur_file : valid_files) {
        if (cur_file_count > kMaxRingBufferFileNum || cur_file.first < delete_files_before) {
            if (unlink(cur_file.second.c_str()) != 0) {
                PLOG(ERROR) << "Error deleting file";
                success = false;
            }
            cur_file_count--;
        } else {
            break;
        }
    }
    return success;
}

// Helper function for |cpioArchiveFilesInDir|
bool cpioWriteHeader(int out_fd, struct stat& st, const char* file_name, size_t file_name_len) {
    const int buf_size = 32 * 1024;
    std::array<char, buf_size> read_buf;
    ssize_t llen = snprintf(
            read_buf.data(), buf_size, "%s%08X%08X%08X%08X%08X%08X%08X%08X%08X%08X%08X%08X%08X",
            kCpioMagic, static_cast<int>(st.st_ino), st.st_mode, st.st_uid, st.st_gid,
            static_cast<int>(st.st_nlink), static_cast<int>(st.st_mtime),
            static_cast<int>(st.st_size), major(st.st_dev), minor(st.st_dev), major(st.st_rdev),
            minor(st.st_rdev), static_cast<uint32_t>(file_name_len), 0);
    if (write(out_fd, read_buf.data(), llen < buf_size ? llen : buf_size - 1) == -1) {
        PLOG(ERROR) << "Error writing cpio header to file " << file_name;
        return false;
    }
    if (write(out_fd, file_name, file_name_len) == -1) {
        PLOG(ERROR) << "Error writing filename to file " << file_name;
        return false;
    }

    // NUL Pad header up to 4 multiple bytes.
    llen = (llen + file_name_len) % 4;
    if (llen != 0) {
        const uint32_t zero = 0;
        if (write(out_fd, &zero, 4 - llen) == -1) {
            PLOG(ERROR) << "Error padding 0s to file " << file_name;
            return false;
        }
    }
    return true;
}

// Helper function for |cpioArchiveFilesInDir|
size_t cpioWriteFileContent(int fd_read, int out_fd, struct stat& st) {
    // writing content of file
    std::array<char, 32 * 1024> read_buf;
    ssize_t llen = st.st_size;
    size_t n_error = 0;
    while (llen > 0) {
        ssize_t bytes_read = read(fd_read, read_buf.data(), read_buf.size());
        if (bytes_read == -1) {
            PLOG(ERROR) << "Error reading file";
            return ++n_error;
        }
        llen -= bytes_read;
        if (write(out_fd, read_buf.data(), bytes_read) == -1) {
            PLOG(ERROR) << "Error writing data to file";
            return ++n_error;
        }
        if (bytes_read == 0) {  // this should never happen, but just in case
                                // to unstuck from while loop
            PLOG(ERROR) << "Unexpected read result";
            n_error++;
            break;
        }
    }
    llen = st.st_size % 4;
    if (llen != 0) {
        const uint32_t zero = 0;
        if (write(out_fd, &zero, 4 - llen) == -1) {
            PLOG(ERROR) << "Error padding 0s to file";
            return ++n_error;
        }
    }
    return n_error;
}

// Helper function for |cpioArchiveFilesInDir|
bool cpioWriteFileTrailer(int out_fd) {
    const int buf_size = 4096;
    std::array<char, buf_size> read_buf;
    read_buf.fill(0);
    ssize_t llen = snprintf(read_buf.data(), 4096, "070701%040X%056X%08XTRAILER!!!", 1, 0x0b, 0);
    if (write(out_fd, read_buf.data(), (llen < buf_size ? llen : buf_size - 1) + 4) == -1) {
        PLOG(ERROR) << "Error writing trailing bytes";
        return false;
    }
    return true;
}

// Archives all files in |input_dir| and writes result into |out_fd|
// Logic obtained from //external/toybox/toys/posix/cpio.c "Output cpio archive"
// portion
size_t cpioArchiveFilesInDir(int out_fd, const char* input_dir) {
    struct dirent* dp;
    size_t n_error = 0;
    std::unique_ptr<DIR, decltype(&closedir)> dir_dump(opendir(input_dir), closedir);
    if (!dir_dump) {
        PLOG(ERROR) << "Failed to open directory";
        return ++n_error;
    }
    while ((dp = readdir(dir_dump.get()))) {
        if (dp->d_type != DT_REG) {
            continue;
        }
        std::string cur_file_name(dp->d_name);
        struct stat st;
        const std::string cur_file_path = kTombstoneFolderPath + cur_file_name;
        if (stat(cur_file_path.c_str(), &st) == -1) {
            PLOG(ERROR) << "Failed to get file stat for " << cur_file_path;
            n_error++;
            continue;
        }
        const int fd_read = open(cur_file_path.c_str(), O_RDONLY);
        if (fd_read == -1) {
            PLOG(ERROR) << "Failed to open file " << cur_file_path;
            n_error++;
            continue;
        }
        std::string file_name_with_last_modified_time =
                cur_file_name + "-" + std::to_string(st.st_mtime);
        // string.size() does not include the null terminator. The cpio FreeBSD
        // file header expects the null character to be included in the length.
        const size_t file_name_len = file_name_with_last_modified_time.size() + 1;
        unique_fd file_auto_closer(fd_read);
        if (!cpioWriteHeader(out_fd, st, file_name_with_last_modified_time.c_str(),
                             file_name_len)) {
            return ++n_error;
        }
        size_t write_error = cpioWriteFileContent(fd_read, out_fd, st);
        if (write_error) {
            return n_error + write_error;
        }
    }
    if (!cpioWriteFileTrailer(out_fd)) {
        return ++n_error;
    }
    return n_error;
}

// Helper function to create a non-const char*.
std::vector<char> makeCharVec(const std::string& str) {
    std::vector<char> vec(str.size() + 1);
    vec.assign(str.begin(), str.end());
    vec.push_back('\0');
    return vec;
}

}  // namespace

namespace aidl {
namespace android {
namespace hardware {
namespace wifi {
using aidl_return_util::validateAndCall;
using aidl_return_util::validateAndCallWithLock;

WifiChip::WifiChip(int32_t chip_id, bool is_primary,
                   const std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal,
                   const std::weak_ptr<mode_controller::WifiModeController> mode_controller,
                   const std::shared_ptr<iface_util::WifiIfaceUtil> iface_util,
                   const std::weak_ptr<feature_flags::WifiFeatureFlags> feature_flags,
                   const std::function<void(const std::string&)>& handler,
                   bool using_dynamic_iface_combination)
    : chip_id_(chip_id),
      legacy_hal_(legacy_hal),
      mode_controller_(mode_controller),
      iface_util_(iface_util),
      is_valid_(true),
      current_mode_id_(feature_flags::chip_mode_ids::kInvalid),
      modes_(feature_flags.lock()->getChipModes(is_primary)),
      debug_ring_buffer_cb_registered_(false),
      using_dynamic_iface_combination_(using_dynamic_iface_combination),
      subsystemCallbackHandler_(handler) {
    setActiveWlanIfaceNameProperty(kNoActiveWlanIfaceNamePropertyValue);
}

void WifiChip::retrieveDynamicIfaceCombination() {
    if (using_dynamic_iface_combination_) return;

    legacy_hal::wifi_iface_concurrency_matrix legacy_matrix;
    legacy_hal::wifi_error legacy_status;

    std::tie(legacy_status, legacy_matrix) =
            legacy_hal_.lock()->getSupportedIfaceConcurrencyMatrix();
    if (legacy_status != legacy_hal::WIFI_SUCCESS) {
        LOG(ERROR) << "Failed to get SupportedIfaceCombinations matrix from legacy HAL: "
                   << legacyErrorToString(legacy_status);
        return;
    }

    IWifiChip::ChipMode aidl_chip_mode;
    if (!aidl_struct_util::convertLegacyIfaceCombinationsMatrixToChipMode(legacy_matrix,
                                                                          &aidl_chip_mode)) {
        LOG(ERROR) << "Failed convertLegacyIfaceCombinationsMatrixToChipMode() ";
        return;
    }

    LOG(INFO) << "Reloading iface concurrency combination from driver";
    aidl_chip_mode.id = feature_flags::chip_mode_ids::kV3;
    modes_.clear();
    modes_.push_back(aidl_chip_mode);
    using_dynamic_iface_combination_ = true;
}

std::shared_ptr<WifiChip> WifiChip::create(
        int32_t chip_id, bool is_primary, const std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal,
        const std::weak_ptr<mode_controller::WifiModeController> mode_controller,
        const std::shared_ptr<iface_util::WifiIfaceUtil> iface_util,
        const std::weak_ptr<feature_flags::WifiFeatureFlags> feature_flags,
        const std::function<void(const std::string&)>& handler,
        bool using_dynamic_iface_combination) {
    std::shared_ptr<WifiChip> ptr = ndk::SharedRefBase::make<WifiChip>(
            chip_id, is_primary, legacy_hal, mode_controller, iface_util, feature_flags, handler,
            using_dynamic_iface_combination);
    std::weak_ptr<WifiChip> weak_ptr_this(ptr);
    ptr->setWeakPtr(weak_ptr_this);
    return ptr;
}

void WifiChip::invalidate() {
    if (!writeRingbufferFilesInternal()) {
        LOG(ERROR) << "Error writing files to flash";
    }
    invalidateAndRemoveAllIfaces();
    setActiveWlanIfaceNameProperty(kNoActiveWlanIfaceNamePropertyValue);
    legacy_hal_.reset();
    event_cb_handler_.invalidate();
    is_valid_ = false;
}

void WifiChip::setWeakPtr(std::weak_ptr<WifiChip> ptr) {
    weak_ptr_this_ = ptr;
}

bool WifiChip::isValid() {
    return is_valid_;
}

std::set<std::shared_ptr<IWifiChipEventCallback>> WifiChip::getEventCallbacks() {
    return event_cb_handler_.getCallbacks();
}

ndk::ScopedAStatus WifiChip::getId(int32_t* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID, &WifiChip::getIdInternal,
                           _aidl_return);
}

ndk::ScopedAStatus WifiChip::registerEventCallback(
        const std::shared_ptr<IWifiChipEventCallback>& event_callback) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::registerEventCallbackInternal, event_callback);
}

ndk::ScopedAStatus WifiChip::getFeatureSet(int32_t* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::getFeatureSetInternal, _aidl_return);
}

ndk::ScopedAStatus WifiChip::getAvailableModes(std::vector<IWifiChip::ChipMode>* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::getAvailableModesInternal, _aidl_return);
}

ndk::ScopedAStatus WifiChip::configureChip(int32_t in_modeId) {
    return validateAndCallWithLock(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                                   &WifiChip::configureChipInternal, in_modeId);
}

ndk::ScopedAStatus WifiChip::getMode(int32_t* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::getModeInternal, _aidl_return);
}

ndk::ScopedAStatus WifiChip::requestChipDebugInfo(IWifiChip::ChipDebugInfo* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::requestChipDebugInfoInternal, _aidl_return);
}

ndk::ScopedAStatus WifiChip::requestDriverDebugDump(std::vector<uint8_t>* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::requestDriverDebugDumpInternal, _aidl_return);
}

ndk::ScopedAStatus WifiChip::requestFirmwareDebugDump(std::vector<uint8_t>* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::requestFirmwareDebugDumpInternal, _aidl_return);
}

ndk::ScopedAStatus WifiChip::createApIface(std::shared_ptr<IWifiApIface>* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::createApIfaceInternal, _aidl_return);
}

ndk::ScopedAStatus WifiChip::createBridgedApIface(std::shared_ptr<IWifiApIface>* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::createBridgedApIfaceInternal, _aidl_return);
}

ndk::ScopedAStatus WifiChip::getApIfaceNames(std::vector<std::string>* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::getApIfaceNamesInternal, _aidl_return);
}

ndk::ScopedAStatus WifiChip::getApIface(const std::string& in_ifname,
                                        std::shared_ptr<IWifiApIface>* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::getApIfaceInternal, _aidl_return, in_ifname);
}

ndk::ScopedAStatus WifiChip::removeApIface(const std::string& in_ifname) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::removeApIfaceInternal, in_ifname);
}

ndk::ScopedAStatus WifiChip::removeIfaceInstanceFromBridgedApIface(
        const std::string& in_brIfaceName, const std::string& in_ifaceInstanceName) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::removeIfaceInstanceFromBridgedApIfaceInternal, in_brIfaceName,
                           in_ifaceInstanceName);
}

ndk::ScopedAStatus WifiChip::createNanIface(std::shared_ptr<IWifiNanIface>* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::createNanIfaceInternal, _aidl_return);
}

ndk::ScopedAStatus WifiChip::getNanIfaceNames(std::vector<std::string>* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::getNanIfaceNamesInternal, _aidl_return);
}

ndk::ScopedAStatus WifiChip::getNanIface(const std::string& in_ifname,
                                         std::shared_ptr<IWifiNanIface>* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::getNanIfaceInternal, _aidl_return, in_ifname);
}

ndk::ScopedAStatus WifiChip::removeNanIface(const std::string& in_ifname) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::removeNanIfaceInternal, in_ifname);
}

ndk::ScopedAStatus WifiChip::createP2pIface(std::shared_ptr<IWifiP2pIface>* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::createP2pIfaceInternal, _aidl_return);
}

ndk::ScopedAStatus WifiChip::getP2pIfaceNames(std::vector<std::string>* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::getP2pIfaceNamesInternal, _aidl_return);
}

ndk::ScopedAStatus WifiChip::getP2pIface(const std::string& in_ifname,
                                         std::shared_ptr<IWifiP2pIface>* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::getP2pIfaceInternal, _aidl_return, in_ifname);
}

ndk::ScopedAStatus WifiChip::removeP2pIface(const std::string& in_ifname) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::removeP2pIfaceInternal, in_ifname);
}

ndk::ScopedAStatus WifiChip::createStaIface(std::shared_ptr<IWifiStaIface>* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::createStaIfaceInternal, _aidl_return);
}

ndk::ScopedAStatus WifiChip::getStaIfaceNames(std::vector<std::string>* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::getStaIfaceNamesInternal, _aidl_return);
}

ndk::ScopedAStatus WifiChip::getStaIface(const std::string& in_ifname,
                                         std::shared_ptr<IWifiStaIface>* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::getStaIfaceInternal, _aidl_return, in_ifname);
}

ndk::ScopedAStatus WifiChip::removeStaIface(const std::string& in_ifname) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::removeStaIfaceInternal, in_ifname);
}

ndk::ScopedAStatus WifiChip::createRttController(
        const std::shared_ptr<IWifiStaIface>& in_boundIface,
        std::shared_ptr<IWifiRttController>* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::createRttControllerInternal, _aidl_return, in_boundIface);
}

ndk::ScopedAStatus WifiChip::getDebugRingBuffersStatus(
        std::vector<WifiDebugRingBufferStatus>* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::getDebugRingBuffersStatusInternal, _aidl_return);
}

ndk::ScopedAStatus WifiChip::startLoggingToDebugRingBuffer(
        const std::string& in_ringName, WifiDebugRingBufferVerboseLevel in_verboseLevel,
        int32_t in_maxIntervalInSec, int32_t in_minDataSizeInBytes) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::startLoggingToDebugRingBufferInternal, in_ringName,
                           in_verboseLevel, in_maxIntervalInSec, in_minDataSizeInBytes);
}

ndk::ScopedAStatus WifiChip::forceDumpToDebugRingBuffer(const std::string& in_ringName) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::forceDumpToDebugRingBufferInternal, in_ringName);
}

ndk::ScopedAStatus WifiChip::flushRingBufferToFile() {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::flushRingBufferToFileInternal);
}

ndk::ScopedAStatus WifiChip::stopLoggingToDebugRingBuffer() {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::stopLoggingToDebugRingBufferInternal);
}

ndk::ScopedAStatus WifiChip::getDebugHostWakeReasonStats(
        WifiDebugHostWakeReasonStats* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::getDebugHostWakeReasonStatsInternal, _aidl_return);
}

ndk::ScopedAStatus WifiChip::enableDebugErrorAlerts(bool in_enable) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::enableDebugErrorAlertsInternal, in_enable);
}

ndk::ScopedAStatus WifiChip::selectTxPowerScenario(IWifiChip::TxPowerScenario in_scenario) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::selectTxPowerScenarioInternal, in_scenario);
}

ndk::ScopedAStatus WifiChip::resetTxPowerScenario() {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::resetTxPowerScenarioInternal);
}

ndk::ScopedAStatus WifiChip::setLatencyMode(IWifiChip::LatencyMode in_mode) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::setLatencyModeInternal, in_mode);
}

binder_status_t WifiChip::dump(int fd, const char**, uint32_t) {
    {
        std::unique_lock<std::mutex> lk(lock_t);
        for (const auto& item : ringbuffer_map_) {
            forceDumpToDebugRingBufferInternal(item.first);
        }
        // unique_lock unlocked here
    }
    usleep(100 * 1000);  // sleep for 100 milliseconds to wait for
                         // ringbuffer updates.
    if (!writeRingbufferFilesInternal()) {
        LOG(ERROR) << "Error writing files to flash";
    }
    uint32_t n_error = cpioArchiveFilesInDir(fd, kTombstoneFolderPath);
    if (n_error != 0) {
        LOG(ERROR) << n_error << " errors occurred in cpio function";
    }
    fsync(fd);
    return STATUS_OK;
}

ndk::ScopedAStatus WifiChip::setMultiStaPrimaryConnection(const std::string& in_ifName) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::setMultiStaPrimaryConnectionInternal, in_ifName);
}

ndk::ScopedAStatus WifiChip::setMultiStaUseCase(IWifiChip::MultiStaUseCase in_useCase) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::setMultiStaUseCaseInternal, in_useCase);
}

ndk::ScopedAStatus WifiChip::setCoexUnsafeChannels(
        const std::vector<IWifiChip::CoexUnsafeChannel>& in_unsafeChannels,
        int32_t in_restrictions) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::setCoexUnsafeChannelsInternal, in_unsafeChannels,
                           in_restrictions);
}

ndk::ScopedAStatus WifiChip::setCountryCode(const std::array<uint8_t, 2>& in_code) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
                           &WifiChip::setCountryCodeInternal, in_code);
}

ndk::ScopedAStatus WifiChip::getUsableChannels(WifiBand in_band, int32_t in_ifaceModeMask,
                                               int32_t in_filterMask,
                                               std::vector<WifiUsableChannel>* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::getUsableChannelsInternal, _aidl_return, in_band,
                           in_ifaceModeMask, in_filterMask);
}

ndk::ScopedAStatus WifiChip::setAfcChannelAllowance(
        const AfcChannelAllowance& afcChannelAllowance) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::setAfcChannelAllowanceInternal, afcChannelAllowance);
}

ndk::ScopedAStatus WifiChip::triggerSubsystemRestart() {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::triggerSubsystemRestartInternal);
}

ndk::ScopedAStatus WifiChip::getSupportedRadioCombinations(
        std::vector<WifiRadioCombination>* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::getSupportedRadioCombinationsInternal, _aidl_return);
}

ndk::ScopedAStatus WifiChip::getWifiChipCapabilities(WifiChipCapabilities* _aidl_return) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::getWifiChipCapabilitiesInternal, _aidl_return);
}

ndk::ScopedAStatus WifiChip::enableStaChannelForPeerNetwork(int32_t in_channelCategoryEnableFlag) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::enableStaChannelForPeerNetworkInternal,
                           in_channelCategoryEnableFlag);
}

ndk::ScopedAStatus WifiChip::setMloMode(const ChipMloMode in_mode) {
    return validateAndCall(this, WifiStatusCode::ERROR_WIFI_CHIP_INVALID,
                           &WifiChip::setMloModeInternal, in_mode);
}

void WifiChip::invalidateAndRemoveAllIfaces() {
    invalidateAndClearBridgedApAll();
    invalidateAndClearAll(ap_ifaces_);
    invalidateAndClearAll(nan_ifaces_);
    invalidateAndClearAll(p2p_ifaces_);
    invalidateAndClearAll(sta_ifaces_);
    // Since all the ifaces are invalid now, all RTT controller objects
    // using those ifaces also need to be invalidated.
    for (const auto& rtt : rtt_controllers_) {
        rtt->invalidate();
    }
    rtt_controllers_.clear();
}

void WifiChip::invalidateAndRemoveDependencies(const std::string& removed_iface_name) {
    for (auto it = nan_ifaces_.begin(); it != nan_ifaces_.end();) {
        auto nan_iface = *it;
        if (nan_iface->getName() == removed_iface_name) {
            nan_iface->invalidate();
            for (const auto& callback : event_cb_handler_.getCallbacks()) {
                if (!callback->onIfaceRemoved(IfaceType::NAN_IFACE, removed_iface_name).isOk()) {
                    LOG(ERROR) << "Failed to invoke onIfaceRemoved callback";
                }
            }
            it = nan_ifaces_.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = rtt_controllers_.begin(); it != rtt_controllers_.end();) {
        auto rtt = *it;
        if (rtt->getIfaceName() == removed_iface_name) {
            rtt->invalidate();
            it = rtt_controllers_.erase(it);
        } else {
            ++it;
        }
    }
}

std::pair<int32_t, ndk::ScopedAStatus> WifiChip::getIdInternal() {
    return {chip_id_, ndk::ScopedAStatus::ok()};
}

ndk::ScopedAStatus WifiChip::registerEventCallbackInternal(
        const std::shared_ptr<IWifiChipEventCallback>& event_callback) {
    if (!event_cb_handler_.addCallback(event_callback)) {
        return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN);
    }
    return ndk::ScopedAStatus::ok();
}

std::pair<int32_t, ndk::ScopedAStatus> WifiChip::getFeatureSetInternal() {
    legacy_hal::wifi_error legacy_status;
    uint64_t legacy_feature_set;
    uint32_t legacy_logger_feature_set;
    const auto ifname = getFirstActiveWlanIfaceName();
    std::tie(legacy_status, legacy_feature_set) =
            legacy_hal_.lock()->getSupportedFeatureSet(ifname);
    if (legacy_status != legacy_hal::WIFI_SUCCESS) {
        return {0, createWifiStatusFromLegacyError(legacy_status)};
    }
    std::tie(legacy_status, legacy_logger_feature_set) =
            legacy_hal_.lock()->getLoggerSupportedFeatureSet(ifname);
    if (legacy_status != legacy_hal::WIFI_SUCCESS) {
        // some devices don't support querying logger feature set
        legacy_logger_feature_set = 0;
    }
    uint32_t aidl_feature_set;
    if (!aidl_struct_util::convertLegacyChipFeaturesToAidl(legacy_feature_set, &aidl_feature_set)) {
        return {0, createWifiStatus(WifiStatusCode::ERROR_UNKNOWN)};
    }
    return {aidl_feature_set, ndk::ScopedAStatus::ok()};
}

std::pair<std::vector<IWifiChip::ChipMode>, ndk::ScopedAStatus>
WifiChip::getAvailableModesInternal() {
    return {modes_, ndk::ScopedAStatus::ok()};
}

ndk::ScopedAStatus WifiChip::configureChipInternal(
        /* NONNULL */ std::unique_lock<std::recursive_mutex>* lock, int32_t mode_id) {
    if (!isValidModeId(mode_id)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    if (mode_id == current_mode_id_) {
        LOG(DEBUG) << "Already in the specified mode " << mode_id;
        return ndk::ScopedAStatus::ok();
    }
    ndk::ScopedAStatus status = handleChipConfiguration(lock, mode_id);
    if (!status.isOk()) {
        WifiStatusCode errorCode = static_cast<WifiStatusCode>(status.getServiceSpecificError());
        for (const auto& callback : event_cb_handler_.getCallbacks()) {
            if (!callback->onChipReconfigureFailure(errorCode).isOk()) {
                LOG(ERROR) << "Failed to invoke onChipReconfigureFailure callback";
            }
        }
        return status;
    }
    for (const auto& callback : event_cb_handler_.getCallbacks()) {
        if (!callback->onChipReconfigured(mode_id).isOk()) {
            LOG(ERROR) << "Failed to invoke onChipReconfigured callback";
        }
    }
    current_mode_id_ = mode_id;
    LOG(INFO) << "Configured chip in mode " << mode_id;
    setActiveWlanIfaceNameProperty(getFirstActiveWlanIfaceName());

    legacy_hal_.lock()->registerSubsystemRestartCallbackHandler(subsystemCallbackHandler_);

    return status;
}

std::pair<int32_t, ndk::ScopedAStatus> WifiChip::getModeInternal() {
    if (!isValidModeId(current_mode_id_)) {
        return {current_mode_id_, createWifiStatus(WifiStatusCode::ERROR_NOT_AVAILABLE)};
    }
    return {current_mode_id_, ndk::ScopedAStatus::ok()};
}

std::pair<IWifiChip::ChipDebugInfo, ndk::ScopedAStatus> WifiChip::requestChipDebugInfoInternal() {
    IWifiChip::ChipDebugInfo result;
    legacy_hal::wifi_error legacy_status;
    std::string driver_desc;
    const auto ifname = getFirstActiveWlanIfaceName();
    std::tie(legacy_status, driver_desc) = legacy_hal_.lock()->getDriverVersion(ifname);
    if (legacy_status != legacy_hal::WIFI_SUCCESS) {
        LOG(ERROR) << "Failed to get driver version: " << legacyErrorToString(legacy_status);
        ndk::ScopedAStatus status =
                createWifiStatusFromLegacyError(legacy_status, "failed to get driver version");
        return {std::move(result), std::move(status)};
    }
    result.driverDescription = driver_desc.c_str();

    std::string firmware_desc;
    std::tie(legacy_status, firmware_desc) = legacy_hal_.lock()->getFirmwareVersion(ifname);
    if (legacy_status != legacy_hal::WIFI_SUCCESS) {
        LOG(ERROR) << "Failed to get firmware version: " << legacyErrorToString(legacy_status);
        ndk::ScopedAStatus status =
                createWifiStatusFromLegacyError(legacy_status, "failed to get firmware version");
        return {std::move(result), std::move(status)};
    }
    result.firmwareDescription = firmware_desc.c_str();

    return {std::move(result), ndk::ScopedAStatus::ok()};
}

std::pair<std::vector<uint8_t>, ndk::ScopedAStatus> WifiChip::requestDriverDebugDumpInternal() {
    legacy_hal::wifi_error legacy_status;
    std::vector<uint8_t> driver_dump;
    std::tie(legacy_status, driver_dump) =
            legacy_hal_.lock()->requestDriverMemoryDump(getFirstActiveWlanIfaceName());
    if (legacy_status != legacy_hal::WIFI_SUCCESS) {
        LOG(ERROR) << "Failed to get driver debug dump: " << legacyErrorToString(legacy_status);
        return {std::vector<uint8_t>(), createWifiStatusFromLegacyError(legacy_status)};
    }
    return {driver_dump, ndk::ScopedAStatus::ok()};
}

std::pair<std::vector<uint8_t>, ndk::ScopedAStatus> WifiChip::requestFirmwareDebugDumpInternal() {
    legacy_hal::wifi_error legacy_status;
    std::vector<uint8_t> firmware_dump;
    std::tie(legacy_status, firmware_dump) =
            legacy_hal_.lock()->requestFirmwareMemoryDump(getFirstActiveWlanIfaceName());
    if (legacy_status != legacy_hal::WIFI_SUCCESS) {
        LOG(ERROR) << "Failed to get firmware debug dump: " << legacyErrorToString(legacy_status);
        return {std::vector<uint8_t>(), createWifiStatusFromLegacyError(legacy_status)};
    }
    return {firmware_dump, ndk::ScopedAStatus::ok()};
}

ndk::ScopedAStatus WifiChip::createVirtualApInterface(const std::string& apVirtIf) {
    legacy_hal::wifi_error legacy_status;
    legacy_status = legacy_hal_.lock()->createVirtualInterface(
            apVirtIf, aidl_struct_util::convertAidlIfaceTypeToLegacy(IfaceType::AP));
    if (legacy_status != legacy_hal::WIFI_SUCCESS) {
        LOG(ERROR) << "Failed to add interface: " << apVirtIf << " "
                   << legacyErrorToString(legacy_status);
        return createWifiStatusFromLegacyError(legacy_status);
    }
    return ndk::ScopedAStatus::ok();
}

std::shared_ptr<WifiApIface> WifiChip::newWifiApIface(std::string& ifname) {
    std::vector<std::string> ap_instances;
    for (auto const& it : br_ifaces_ap_instances_) {
        if (it.first == ifname) {
            ap_instances = it.second;
        }
    }
    std::shared_ptr<WifiApIface> iface =
            ndk::SharedRefBase::make<WifiApIface>(ifname, ap_instances, legacy_hal_, iface_util_);
    ap_ifaces_.push_back(iface);
    for (const auto& callback : event_cb_handler_.getCallbacks()) {
        if (!callback->onIfaceAdded(IfaceType::AP, ifname).isOk()) {
            LOG(ERROR) << "Failed to invoke onIfaceAdded callback";
        }
    }
    setActiveWlanIfaceNameProperty(getFirstActiveWlanIfaceName());
    return iface;
}

std::pair<std::shared_ptr<IWifiApIface>, ndk::ScopedAStatus> WifiChip::createApIfaceInternal() {
    if (!canCurrentModeSupportConcurrencyTypeWithCurrentTypes(IfaceConcurrencyType::AP)) {
        return {std::shared_ptr<WifiApIface>(),
                createWifiStatus(WifiStatusCode::ERROR_NOT_AVAILABLE)};
    }
    std::string ifname = allocateApIfaceName();
    ndk::ScopedAStatus status = createVirtualApInterface(ifname);
    if (!status.isOk()) {
        return {std::shared_ptr<WifiApIface>(), std::move(status)};
    }
    std::shared_ptr<WifiApIface> iface = newWifiApIface(ifname);
    return {iface, ndk::ScopedAStatus::ok()};
}

std::pair<std::shared_ptr<IWifiApIface>, ndk::ScopedAStatus>
WifiChip::createBridgedApIfaceInternal() {
    if (!canCurrentModeSupportConcurrencyTypeWithCurrentTypes(IfaceConcurrencyType::AP_BRIDGED)) {
        return {nullptr, createWifiStatus(WifiStatusCode::ERROR_NOT_AVAILABLE)};
    }
    std::vector<std::string> ap_instances = allocateBridgedApInstanceNames();
    if (ap_instances.size() < 2) {
        LOG(ERROR) << "Fail to allocate two instances";
        return {nullptr, createWifiStatus(WifiStatusCode::ERROR_NOT_AVAILABLE)};
    }
    std::string br_ifname = kApBridgeIfacePrefix + ap_instances[0];
    for (int i = 0; i < 2; i++) {
        ndk::ScopedAStatus status = createVirtualApInterface(ap_instances[i]);
        if (!status.isOk()) {
            if (i != 0) {  // The failure happened when creating second virtual
                           // iface.
                legacy_hal_.lock()->deleteVirtualInterface(
                        ap_instances.front());  // Remove the first virtual iface.
            }
            return {nullptr, std::move(status)};
        }
    }
    br_ifaces_ap_instances_[br_ifname] = ap_instances;
    if (!iface_util_->createBridge(br_ifname)) {
        LOG(ERROR) << "Failed createBridge - br_name=" << br_ifname.c_str();
        deleteApIface(br_ifname);
        return {nullptr, createWifiStatus(WifiStatusCode::ERROR_NOT_AVAILABLE)};
    }
    for (auto const& instance : ap_instances) {
        // Bind ap instance interface to AP bridge
        if (!iface_util_->addIfaceToBridge(br_ifname, instance)) {
            LOG(ERROR) << "Failed add if to Bridge - if_name=" << instance.c_str();
            deleteApIface(br_ifname);
            return {nullptr, createWifiStatus(WifiStatusCode::ERROR_NOT_AVAILABLE)};
        }
    }
    std::shared_ptr<WifiApIface> iface = newWifiApIface(br_ifname);
    return {iface, ndk::ScopedAStatus::ok()};
}

std::pair<std::vector<std::string>, ndk::ScopedAStatus> WifiChip::getApIfaceNamesInternal() {
    if (ap_ifaces_.empty()) {
        return {std::vector<std::string>(), ndk::ScopedAStatus::ok()};
    }
    return {getNames(ap_ifaces_), ndk::ScopedAStatus::ok()};
}

std::pair<std::shared_ptr<IWifiApIface>, ndk::ScopedAStatus> WifiChip::getApIfaceInternal(
        const std::string& ifname) {
    const auto iface = findUsingName(ap_ifaces_, ifname);
    if (!iface.get()) {
        return {nullptr, createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS)};
    }
    return {iface, ndk::ScopedAStatus::ok()};
}

ndk::ScopedAStatus WifiChip::removeApIfaceInternal(const std::string& ifname) {
    const auto iface = findUsingName(ap_ifaces_, ifname);
    if (!iface.get()) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    // Invalidate & remove any dependent objects first.
    // Note: This is probably not required because we never create
    // nan/rtt objects over AP iface. But, there is no harm to do it
    // here and not make that assumption all over the place.
    invalidateAndRemoveDependencies(ifname);
    deleteApIface(ifname);
    invalidateAndClear(ap_ifaces_, iface);
    for (const auto& callback : event_cb_handler_.getCallbacks()) {
        if (!callback->onIfaceRemoved(IfaceType::AP, ifname).isOk()) {
            LOG(ERROR) << "Failed to invoke onIfaceRemoved callback";
        }
    }
    setActiveWlanIfaceNameProperty(getFirstActiveWlanIfaceName());
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus WifiChip::removeIfaceInstanceFromBridgedApIfaceInternal(
        const std::string& ifname, const std::string& ifInstanceName) {
    const auto iface = findUsingName(ap_ifaces_, ifname);
    if (!iface.get() || ifInstanceName.empty()) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    // Requires to remove one of the instance in bridge mode
    for (auto const& it : br_ifaces_ap_instances_) {
        if (it.first == ifname) {
            std::vector<std::string> ap_instances = it.second;
            for (auto const& iface : ap_instances) {
                if (iface == ifInstanceName) {
                    if (!iface_util_->removeIfaceFromBridge(it.first, iface)) {
                        LOG(ERROR) << "Failed to remove interface: " << ifInstanceName << " from "
                                   << ifname;
                        return createWifiStatus(WifiStatusCode::ERROR_NOT_AVAILABLE);
                    }
                    legacy_hal::wifi_error legacy_status =
                            legacy_hal_.lock()->deleteVirtualInterface(iface);
                    if (legacy_status != legacy_hal::WIFI_SUCCESS) {
                        LOG(ERROR) << "Failed to del interface: " << iface << " "
                                   << legacyErrorToString(legacy_status);
                        return createWifiStatusFromLegacyError(legacy_status);
                    }
                    ap_instances.erase(
                            std::remove(ap_instances.begin(), ap_instances.end(), ifInstanceName),
                            ap_instances.end());
                    br_ifaces_ap_instances_[ifname] = ap_instances;
                    break;
                }
            }
            break;
        }
    }
    iface->removeInstance(ifInstanceName);
    setActiveWlanIfaceNameProperty(getFirstActiveWlanIfaceName());

    return ndk::ScopedAStatus::ok();
}

std::pair<std::shared_ptr<IWifiNanIface>, ndk::ScopedAStatus> WifiChip::createNanIfaceInternal() {
    if (!canCurrentModeSupportConcurrencyTypeWithCurrentTypes(IfaceConcurrencyType::NAN_IFACE)) {
        return {nullptr, createWifiStatus(WifiStatusCode::ERROR_NOT_AVAILABLE)};
    }
    bool is_dedicated_iface = true;
    std::string ifname = getPredefinedNanIfaceName();
    if (ifname.empty() || !iface_util_->ifNameToIndex(ifname)) {
        // Use the first shared STA iface (wlan0) if a dedicated aware iface is
        // not defined.
        ifname = getFirstActiveWlanIfaceName();
        is_dedicated_iface = false;
    }
    std::shared_ptr<WifiNanIface> iface =
            WifiNanIface::create(ifname, is_dedicated_iface, legacy_hal_, iface_util_);
    nan_ifaces_.push_back(iface);
    for (const auto& callback : event_cb_handler_.getCallbacks()) {
        if (!callback->onIfaceAdded(IfaceType::NAN_IFACE, ifname).isOk()) {
            LOG(ERROR) << "Failed to invoke onIfaceAdded callback";
        }
    }
    return {iface, ndk::ScopedAStatus::ok()};
}

std::pair<std::vector<std::string>, ndk::ScopedAStatus> WifiChip::getNanIfaceNamesInternal() {
    if (nan_ifaces_.empty()) {
        return {std::vector<std::string>(), ndk::ScopedAStatus::ok()};
    }
    return {getNames(nan_ifaces_), ndk::ScopedAStatus::ok()};
}

std::pair<std::shared_ptr<IWifiNanIface>, ndk::ScopedAStatus> WifiChip::getNanIfaceInternal(
        const std::string& ifname) {
    const auto iface = findUsingName(nan_ifaces_, ifname);
    if (!iface.get()) {
        return {nullptr, createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS)};
    }
    return {iface, ndk::ScopedAStatus::ok()};
}

ndk::ScopedAStatus WifiChip::removeNanIfaceInternal(const std::string& ifname) {
    const auto iface = findUsingName(nan_ifaces_, ifname);
    if (!iface.get()) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    invalidateAndClear(nan_ifaces_, iface);
    for (const auto& callback : event_cb_handler_.getCallbacks()) {
        if (!callback->onIfaceRemoved(IfaceType::NAN_IFACE, ifname).isOk()) {
            LOG(ERROR) << "Failed to invoke onIfaceAdded callback";
        }
    }
    return ndk::ScopedAStatus::ok();
}

std::pair<std::shared_ptr<IWifiP2pIface>, ndk::ScopedAStatus> WifiChip::createP2pIfaceInternal() {
    if (!canCurrentModeSupportConcurrencyTypeWithCurrentTypes(IfaceConcurrencyType::P2P)) {
        return {nullptr, createWifiStatus(WifiStatusCode::ERROR_NOT_AVAILABLE)};
    }
    std::string ifname = getPredefinedP2pIfaceName();
    std::shared_ptr<WifiP2pIface> iface =
            ndk::SharedRefBase::make<WifiP2pIface>(ifname, legacy_hal_);
    p2p_ifaces_.push_back(iface);
    for (const auto& callback : event_cb_handler_.getCallbacks()) {
        if (!callback->onIfaceAdded(IfaceType::P2P, ifname).isOk()) {
            LOG(ERROR) << "Failed to invoke onIfaceAdded callback";
        }
    }
    return {iface, ndk::ScopedAStatus::ok()};
}

std::pair<std::vector<std::string>, ndk::ScopedAStatus> WifiChip::getP2pIfaceNamesInternal() {
    if (p2p_ifaces_.empty()) {
        return {std::vector<std::string>(), ndk::ScopedAStatus::ok()};
    }
    return {getNames(p2p_ifaces_), ndk::ScopedAStatus::ok()};
}

std::pair<std::shared_ptr<IWifiP2pIface>, ndk::ScopedAStatus> WifiChip::getP2pIfaceInternal(
        const std::string& ifname) {
    const auto iface = findUsingName(p2p_ifaces_, ifname);
    if (!iface.get()) {
        return {nullptr, createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS)};
    }
    return {iface, ndk::ScopedAStatus::ok()};
}

ndk::ScopedAStatus WifiChip::removeP2pIfaceInternal(const std::string& ifname) {
    const auto iface = findUsingName(p2p_ifaces_, ifname);
    if (!iface.get()) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    invalidateAndClear(p2p_ifaces_, iface);
    for (const auto& callback : event_cb_handler_.getCallbacks()) {
        if (!callback->onIfaceRemoved(IfaceType::P2P, ifname).isOk()) {
            LOG(ERROR) << "Failed to invoke onIfaceRemoved callback";
        }
    }
    return ndk::ScopedAStatus::ok();
}

std::pair<std::shared_ptr<IWifiStaIface>, ndk::ScopedAStatus> WifiChip::createStaIfaceInternal() {
    if (!canCurrentModeSupportConcurrencyTypeWithCurrentTypes(IfaceConcurrencyType::STA)) {
        return {nullptr, createWifiStatus(WifiStatusCode::ERROR_NOT_AVAILABLE)};
    }
    std::string ifname = allocateStaIfaceName();
    legacy_hal::wifi_error legacy_status = legacy_hal_.lock()->createVirtualInterface(
            ifname, aidl_struct_util::convertAidlIfaceTypeToLegacy(IfaceType::STA));
    if (legacy_status != legacy_hal::WIFI_SUCCESS) {
        LOG(ERROR) << "Failed to add interface: " << ifname << " "
                   << legacyErrorToString(legacy_status);
        return {nullptr, createWifiStatusFromLegacyError(legacy_status)};
    }
    std::shared_ptr<WifiStaIface> iface = WifiStaIface::create(ifname, legacy_hal_, iface_util_);
    sta_ifaces_.push_back(iface);
    for (const auto& callback : event_cb_handler_.getCallbacks()) {
        if (!callback->onIfaceAdded(IfaceType::STA, ifname).isOk()) {
            LOG(ERROR) << "Failed to invoke onIfaceAdded callback";
        }
    }
    setActiveWlanIfaceNameProperty(getFirstActiveWlanIfaceName());
    return {iface, ndk::ScopedAStatus::ok()};
}

std::pair<std::vector<std::string>, ndk::ScopedAStatus> WifiChip::getStaIfaceNamesInternal() {
    if (sta_ifaces_.empty()) {
        return {std::vector<std::string>(), ndk::ScopedAStatus::ok()};
    }
    return {getNames(sta_ifaces_), ndk::ScopedAStatus::ok()};
}

std::pair<std::shared_ptr<IWifiStaIface>, ndk::ScopedAStatus> WifiChip::getStaIfaceInternal(
        const std::string& ifname) {
    const auto iface = findUsingName(sta_ifaces_, ifname);
    if (!iface.get()) {
        return {nullptr, createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS)};
    }
    return {iface, ndk::ScopedAStatus::ok()};
}

ndk::ScopedAStatus WifiChip::removeStaIfaceInternal(const std::string& ifname) {
    const auto iface = findUsingName(sta_ifaces_, ifname);
    if (!iface.get()) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    // Invalidate & remove any dependent objects first.
    invalidateAndRemoveDependencies(ifname);
    legacy_hal::wifi_error legacy_status = legacy_hal_.lock()->deleteVirtualInterface(ifname);
    if (legacy_status != legacy_hal::WIFI_SUCCESS) {
        LOG(ERROR) << "Failed to remove interface: " << ifname << " "
                   << legacyErrorToString(legacy_status);
    }
    invalidateAndClear(sta_ifaces_, iface);
    for (const auto& callback : event_cb_handler_.getCallbacks()) {
        if (!callback->onIfaceRemoved(IfaceType::STA, ifname).isOk()) {
            LOG(ERROR) << "Failed to invoke onIfaceRemoved callback";
        }
    }
    setActiveWlanIfaceNameProperty(getFirstActiveWlanIfaceName());
    return ndk::ScopedAStatus::ok();
}

std::pair<std::shared_ptr<IWifiRttController>, ndk::ScopedAStatus>
WifiChip::createRttControllerInternal(const std::shared_ptr<IWifiStaIface>& bound_iface) {
    if (sta_ifaces_.size() == 0 &&
        !canCurrentModeSupportConcurrencyTypeWithCurrentTypes(IfaceConcurrencyType::STA)) {
        LOG(ERROR) << "createRttControllerInternal: Chip cannot support STAs "
                      "(and RTT by extension)";
        return {nullptr, createWifiStatus(WifiStatusCode::ERROR_NOT_AVAILABLE)};
    }
    std::shared_ptr<WifiRttController> rtt =
            WifiRttController::create(getFirstActiveWlanIfaceName(), bound_iface, legacy_hal_);
    rtt_controllers_.emplace_back(rtt);
    return {rtt, ndk::ScopedAStatus::ok()};
}

std::pair<std::vector<WifiDebugRingBufferStatus>, ndk::ScopedAStatus>
WifiChip::getDebugRingBuffersStatusInternal() {
    legacy_hal::wifi_error legacy_status;
    std::vector<legacy_hal::wifi_ring_buffer_status> legacy_ring_buffer_status_vec;
    std::tie(legacy_status, legacy_ring_buffer_status_vec) =
            legacy_hal_.lock()->getRingBuffersStatus(getFirstActiveWlanIfaceName());
    if (legacy_status != legacy_hal::WIFI_SUCCESS) {
        return {std::vector<WifiDebugRingBufferStatus>(),
                createWifiStatusFromLegacyError(legacy_status)};
    }
    std::vector<WifiDebugRingBufferStatus> aidl_ring_buffer_status_vec;
    if (!aidl_struct_util::convertLegacyVectorOfDebugRingBufferStatusToAidl(
                legacy_ring_buffer_status_vec, &aidl_ring_buffer_status_vec)) {
        return {std::vector<WifiDebugRingBufferStatus>(),
                createWifiStatus(WifiStatusCode::ERROR_UNKNOWN)};
    }
    return {aidl_ring_buffer_status_vec, ndk::ScopedAStatus::ok()};
}

ndk::ScopedAStatus WifiChip::startLoggingToDebugRingBufferInternal(
        const std::string& ring_name, WifiDebugRingBufferVerboseLevel verbose_level,
        uint32_t max_interval_in_sec, uint32_t min_data_size_in_bytes) {
    ndk::ScopedAStatus status = registerDebugRingBufferCallback();
    if (!status.isOk()) {
        return status;
    }
    legacy_hal::wifi_error legacy_status = legacy_hal_.lock()->startRingBufferLogging(
            getFirstActiveWlanIfaceName(), ring_name,
            static_cast<std::underlying_type<WifiDebugRingBufferVerboseLevel>::type>(verbose_level),
            max_interval_in_sec, min_data_size_in_bytes);
    ringbuffer_map_.insert(
            std::pair<std::string, Ringbuffer>(ring_name, Ringbuffer(kMaxBufferSizeBytes)));
    // if verbose logging enabled, turn up HAL daemon logging as well.
    if (verbose_level < WifiDebugRingBufferVerboseLevel::VERBOSE) {
        ::android::base::SetMinimumLogSeverity(::android::base::DEBUG);
    } else {
        ::android::base::SetMinimumLogSeverity(::android::base::VERBOSE);
    }
    return createWifiStatusFromLegacyError(legacy_status);
}

ndk::ScopedAStatus WifiChip::forceDumpToDebugRingBufferInternal(const std::string& ring_name) {
    ndk::ScopedAStatus status = registerDebugRingBufferCallback();
    if (!status.isOk()) {
        return status;
    }
    legacy_hal::wifi_error legacy_status =
            legacy_hal_.lock()->getRingBufferData(getFirstActiveWlanIfaceName(), ring_name);

    return createWifiStatusFromLegacyError(legacy_status);
}

ndk::ScopedAStatus WifiChip::flushRingBufferToFileInternal() {
    if (!writeRingbufferFilesInternal()) {
        LOG(ERROR) << "Error writing files to flash";
        return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus WifiChip::stopLoggingToDebugRingBufferInternal() {
    legacy_hal::wifi_error legacy_status =
            legacy_hal_.lock()->deregisterRingBufferCallbackHandler(getFirstActiveWlanIfaceName());
    if (legacy_status == legacy_hal::WIFI_SUCCESS) {
        debug_ring_buffer_cb_registered_ = false;
    }
    return createWifiStatusFromLegacyError(legacy_status);
}

std::pair<WifiDebugHostWakeReasonStats, ndk::ScopedAStatus>
WifiChip::getDebugHostWakeReasonStatsInternal() {
    legacy_hal::wifi_error legacy_status;
    legacy_hal::WakeReasonStats legacy_stats;
    std::tie(legacy_status, legacy_stats) =
            legacy_hal_.lock()->getWakeReasonStats(getFirstActiveWlanIfaceName());
    if (legacy_status != legacy_hal::WIFI_SUCCESS) {
        return {WifiDebugHostWakeReasonStats{}, createWifiStatusFromLegacyError(legacy_status)};
    }
    WifiDebugHostWakeReasonStats aidl_stats;
    if (!aidl_struct_util::convertLegacyWakeReasonStatsToAidl(legacy_stats, &aidl_stats)) {
        return {WifiDebugHostWakeReasonStats{}, createWifiStatus(WifiStatusCode::ERROR_UNKNOWN)};
    }
    return {aidl_stats, ndk::ScopedAStatus::ok()};
}

ndk::ScopedAStatus WifiChip::enableDebugErrorAlertsInternal(bool enable) {
    legacy_hal::wifi_error legacy_status;
    if (enable) {
        std::weak_ptr<WifiChip> weak_ptr_this = weak_ptr_this_;
        const auto& on_alert_callback = [weak_ptr_this](int32_t error_code,
                                                        std::vector<uint8_t> debug_data) {
            const auto shared_ptr_this = weak_ptr_this.lock();
            if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                LOG(ERROR) << "Callback invoked on an invalid object";
                return;
            }
            for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                if (!callback->onDebugErrorAlert(error_code, debug_data).isOk()) {
                    LOG(ERROR) << "Failed to invoke onDebugErrorAlert callback";
                }
            }
        };
        legacy_status = legacy_hal_.lock()->registerErrorAlertCallbackHandler(
                getFirstActiveWlanIfaceName(), on_alert_callback);
    } else {
        legacy_status = legacy_hal_.lock()->deregisterErrorAlertCallbackHandler(
                getFirstActiveWlanIfaceName());
    }
    return createWifiStatusFromLegacyError(legacy_status);
}

ndk::ScopedAStatus WifiChip::selectTxPowerScenarioInternal(IWifiChip::TxPowerScenario scenario) {
    auto legacy_status = legacy_hal_.lock()->selectTxPowerScenario(
            getFirstActiveWlanIfaceName(),
            aidl_struct_util::convertAidlTxPowerScenarioToLegacy(scenario));
    return createWifiStatusFromLegacyError(legacy_status);
}

ndk::ScopedAStatus WifiChip::resetTxPowerScenarioInternal() {
    auto legacy_status = legacy_hal_.lock()->resetTxPowerScenario(getFirstActiveWlanIfaceName());
    return createWifiStatusFromLegacyError(legacy_status);
}

ndk::ScopedAStatus WifiChip::setLatencyModeInternal(IWifiChip::LatencyMode mode) {
    auto legacy_status = legacy_hal_.lock()->setLatencyMode(
            getFirstActiveWlanIfaceName(), aidl_struct_util::convertAidlLatencyModeToLegacy(mode));
    return createWifiStatusFromLegacyError(legacy_status);
}

ndk::ScopedAStatus WifiChip::setMultiStaPrimaryConnectionInternal(const std::string& ifname) {
    auto legacy_status = legacy_hal_.lock()->multiStaSetPrimaryConnection(ifname);
    return createWifiStatusFromLegacyError(legacy_status);
}

ndk::ScopedAStatus WifiChip::setMultiStaUseCaseInternal(IWifiChip::MultiStaUseCase use_case) {
    auto legacy_status = legacy_hal_.lock()->multiStaSetUseCase(
            aidl_struct_util::convertAidlMultiStaUseCaseToLegacy(use_case));
    return createWifiStatusFromLegacyError(legacy_status);
}

ndk::ScopedAStatus WifiChip::setCoexUnsafeChannelsInternal(
        std::vector<IWifiChip::CoexUnsafeChannel> unsafe_channels, int32_t aidl_restrictions) {
    std::vector<legacy_hal::wifi_coex_unsafe_channel> legacy_unsafe_channels;
    if (!aidl_struct_util::convertAidlVectorOfCoexUnsafeChannelToLegacy(unsafe_channels,
                                                                        &legacy_unsafe_channels)) {
        return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    uint32_t legacy_restrictions = 0;
    if (aidl_restrictions & static_cast<uint32_t>(CoexRestriction::WIFI_DIRECT)) {
        legacy_restrictions |= legacy_hal::wifi_coex_restriction::WIFI_DIRECT;
    }
    if (aidl_restrictions & static_cast<uint32_t>(CoexRestriction::SOFTAP)) {
        legacy_restrictions |= legacy_hal::wifi_coex_restriction::SOFTAP;
    }
    if (aidl_restrictions & static_cast<uint32_t>(CoexRestriction::WIFI_AWARE)) {
        legacy_restrictions |= legacy_hal::wifi_coex_restriction::WIFI_AWARE;
    }
    auto legacy_status =
            legacy_hal_.lock()->setCoexUnsafeChannels(legacy_unsafe_channels, legacy_restrictions);
    return createWifiStatusFromLegacyError(legacy_status);
}

ndk::ScopedAStatus WifiChip::setCountryCodeInternal(const std::array<uint8_t, 2>& code) {
    auto legacy_status = legacy_hal_.lock()->setCountryCode(getFirstActiveWlanIfaceName(), code);
    return createWifiStatusFromLegacyError(legacy_status);
}

std::pair<std::vector<WifiUsableChannel>, ndk::ScopedAStatus> WifiChip::getUsableChannelsInternal(
        WifiBand band, int32_t ifaceModeMask, int32_t filterMask) {
    legacy_hal::wifi_error legacy_status;
    std::vector<legacy_hal::wifi_usable_channel> legacy_usable_channels;
    std::tie(legacy_status, legacy_usable_channels) = legacy_hal_.lock()->getUsableChannels(
            aidl_struct_util::convertAidlWifiBandToLegacyMacBand(band),
            aidl_struct_util::convertAidlWifiIfaceModeToLegacy(ifaceModeMask),
            aidl_struct_util::convertAidlUsableChannelFilterToLegacy(filterMask));

    if (legacy_status != legacy_hal::WIFI_SUCCESS) {
        return {std::vector<WifiUsableChannel>(), createWifiStatusFromLegacyError(legacy_status)};
    }
    std::vector<WifiUsableChannel> aidl_usable_channels;
    if (!aidl_struct_util::convertLegacyWifiUsableChannelsToAidl(legacy_usable_channels,
                                                                 &aidl_usable_channels)) {
        return {std::vector<WifiUsableChannel>(), createWifiStatus(WifiStatusCode::ERROR_UNKNOWN)};
    }
    return {aidl_usable_channels, ndk::ScopedAStatus::ok()};
}

ndk::ScopedAStatus WifiChip::setAfcChannelAllowanceInternal(
        const AfcChannelAllowance& afcChannelAllowance) {
    LOG(INFO) << "setAfcChannelAllowance is not yet supported. availableAfcFrequencyInfos size="
              << afcChannelAllowance.availableAfcFrequencyInfos.size()
              << " availableAfcChannelInfos size="
              << afcChannelAllowance.availableAfcChannelInfos.size()
              << " availabilityExpireTimeMs=" << afcChannelAllowance.availabilityExpireTimeMs;
    return createWifiStatus(WifiStatusCode::ERROR_NOT_SUPPORTED);
}

std::pair<std::vector<WifiRadioCombination>, ndk::ScopedAStatus>
WifiChip::getSupportedRadioCombinationsInternal() {
    legacy_hal::wifi_error legacy_status;
    legacy_hal::wifi_radio_combination_matrix* legacy_matrix;
    std::vector<WifiRadioCombination> aidl_combinations;

    std::tie(legacy_status, legacy_matrix) =
            legacy_hal_.lock()->getSupportedRadioCombinationsMatrix();
    if (legacy_status != legacy_hal::WIFI_SUCCESS) {
        LOG(ERROR) << "Failed to get SupportedRadioCombinations matrix from legacy HAL: "
                   << legacyErrorToString(legacy_status);
        if (legacy_matrix != nullptr) {
            free(legacy_matrix);
        }
        return {aidl_combinations, createWifiStatusFromLegacyError(legacy_status)};
    }

    if (!aidl_struct_util::convertLegacyRadioCombinationsMatrixToAidl(legacy_matrix,
                                                                      &aidl_combinations)) {
        LOG(ERROR) << "Failed convertLegacyRadioCombinationsMatrixToAidl() ";
        if (legacy_matrix != nullptr) {
            free(legacy_matrix);
        }
        return {aidl_combinations, createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS)};
    }

    if (legacy_matrix != nullptr) {
        free(legacy_matrix);
    }
    return {aidl_combinations, ndk::ScopedAStatus::ok()};
}

std::pair<WifiChipCapabilities, ndk::ScopedAStatus> WifiChip::getWifiChipCapabilitiesInternal() {
    legacy_hal::wifi_error legacy_status;
    legacy_hal::wifi_chip_capabilities legacy_chip_capabilities;
    std::tie(legacy_status, legacy_chip_capabilities) =
            legacy_hal_.lock()->getWifiChipCapabilities();
    if (legacy_status != legacy_hal::WIFI_SUCCESS) {
        LOG(ERROR) << "Failed to get chip capabilities from legacy HAL: "
                   << legacyErrorToString(legacy_status);
        return {WifiChipCapabilities(), createWifiStatusFromLegacyError(legacy_status)};
    }
    WifiChipCapabilities aidl_chip_capabilities;
    if (!aidl_struct_util::convertLegacyWifiChipCapabilitiesToAidl(legacy_chip_capabilities,
                                                                   aidl_chip_capabilities)) {
        LOG(ERROR) << "Failed convertLegacyWifiChipCapabilitiesToAidl() ";
        return {WifiChipCapabilities(), createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS)};
    }

    return {aidl_chip_capabilities, ndk::ScopedAStatus::ok()};
}

ndk::ScopedAStatus WifiChip::enableStaChannelForPeerNetworkInternal(
        int32_t channelCategoryEnableFlag) {
    auto legacy_status = legacy_hal_.lock()->enableStaChannelForPeerNetwork(
            aidl_struct_util::convertAidlChannelCategoryToLegacy(channelCategoryEnableFlag));
    return createWifiStatusFromLegacyError(legacy_status);
}

ndk::ScopedAStatus WifiChip::triggerSubsystemRestartInternal() {
    auto legacy_status = legacy_hal_.lock()->triggerSubsystemRestart();
    return createWifiStatusFromLegacyError(legacy_status);
}

ndk::ScopedAStatus WifiChip::handleChipConfiguration(
        /* NONNULL */ std::unique_lock<std::recursive_mutex>* lock, int32_t mode_id) {
    // If the chip is already configured in a different mode, stop
    // the legacy HAL and then start it after firmware mode change.
    if (isValidModeId(current_mode_id_)) {
        LOG(INFO) << "Reconfiguring chip from mode " << current_mode_id_ << " to mode " << mode_id;
        invalidateAndRemoveAllIfaces();
        legacy_hal::wifi_error legacy_status = legacy_hal_.lock()->stop(lock, []() {});
        if (legacy_status != legacy_hal::WIFI_SUCCESS) {
            LOG(ERROR) << "Failed to stop legacy HAL: " << legacyErrorToString(legacy_status);
            return createWifiStatusFromLegacyError(legacy_status);
        }
    }
    // Firmware mode change not needed for V2 devices.
    bool success = true;
    if (mode_id == feature_flags::chip_mode_ids::kV1Sta) {
        success = mode_controller_.lock()->changeFirmwareMode(IfaceType::STA);
    } else if (mode_id == feature_flags::chip_mode_ids::kV1Ap) {
        success = mode_controller_.lock()->changeFirmwareMode(IfaceType::AP);
    }
    if (!success) {
        return createWifiStatus(WifiStatusCode::ERROR_UNKNOWN);
    }
    legacy_hal::wifi_error legacy_status = legacy_hal_.lock()->start();
    if (legacy_status != legacy_hal::WIFI_SUCCESS) {
        LOG(ERROR) << "Failed to start legacy HAL: " << legacyErrorToString(legacy_status);
        return createWifiStatusFromLegacyError(legacy_status);
    }
    // Every time the HAL is restarted, we need to register the
    // radio mode change callback.
    ndk::ScopedAStatus status = registerRadioModeChangeCallback();
    if (!status.isOk()) {
        // This is probably not a critical failure?
        LOG(ERROR) << "Failed to register radio mode change callback";
    }
    // Extract and save the version information into property.
    std::pair<IWifiChip::ChipDebugInfo, ndk::ScopedAStatus> version_info;
    version_info = WifiChip::requestChipDebugInfoInternal();
    if (version_info.second.isOk()) {
        property_set("vendor.wlan.firmware.version",
                     version_info.first.firmwareDescription.c_str());
        property_set("vendor.wlan.driver.version", version_info.first.driverDescription.c_str());
    }
    // Get the driver supported interface combination.
    retrieveDynamicIfaceCombination();

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus WifiChip::registerDebugRingBufferCallback() {
    if (debug_ring_buffer_cb_registered_) {
        return ndk::ScopedAStatus::ok();
    }

    std::weak_ptr<WifiChip> weak_ptr_this = weak_ptr_this_;
    const auto& on_ring_buffer_data_callback =
            [weak_ptr_this](const std::string& name, const std::vector<uint8_t>& data,
                            const legacy_hal::wifi_ring_buffer_status& status) {
                const auto shared_ptr_this = weak_ptr_this.lock();
                if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                    LOG(ERROR) << "Callback invoked on an invalid object";
                    return;
                }
                WifiDebugRingBufferStatus aidl_status;
                Ringbuffer::AppendStatus appendstatus;
                if (!aidl_struct_util::convertLegacyDebugRingBufferStatusToAidl(status,
                                                                                &aidl_status)) {
                    LOG(ERROR) << "Error converting ring buffer status";
                    return;
                }
                {
                    std::unique_lock<std::mutex> lk(shared_ptr_this->lock_t);
                    const auto& target = shared_ptr_this->ringbuffer_map_.find(name);
                    if (target != shared_ptr_this->ringbuffer_map_.end()) {
                        Ringbuffer& cur_buffer = target->second;
                        appendstatus = cur_buffer.append(data);
                    } else {
                        LOG(ERROR) << "Ringname " << name << " not found";
                        return;
                    }
                    // unique_lock unlocked here
                }
                if (appendstatus == Ringbuffer::AppendStatus::FAIL_RING_BUFFER_CORRUPTED) {
                    LOG(ERROR) << "Ringname " << name << " is corrupted. Clear the ring buffer";
                    shared_ptr_this->writeRingbufferFilesInternal();
                    return;
                }
            };
    legacy_hal::wifi_error legacy_status = legacy_hal_.lock()->registerRingBufferCallbackHandler(
            getFirstActiveWlanIfaceName(), on_ring_buffer_data_callback);

    if (legacy_status == legacy_hal::WIFI_SUCCESS) {
        debug_ring_buffer_cb_registered_ = true;
    }
    return createWifiStatusFromLegacyError(legacy_status);
}

ndk::ScopedAStatus WifiChip::registerRadioModeChangeCallback() {
    std::weak_ptr<WifiChip> weak_ptr_this = weak_ptr_this_;
    const auto& on_radio_mode_change_callback =
            [weak_ptr_this](const std::vector<legacy_hal::WifiMacInfo>& mac_infos) {
                const auto shared_ptr_this = weak_ptr_this.lock();
                if (!shared_ptr_this.get() || !shared_ptr_this->isValid()) {
                    LOG(ERROR) << "Callback invoked on an invalid object";
                    return;
                }
                std::vector<IWifiChipEventCallback::RadioModeInfo> aidl_radio_mode_infos;
                if (!aidl_struct_util::convertLegacyWifiMacInfosToAidl(mac_infos,
                                                                       &aidl_radio_mode_infos)) {
                    LOG(ERROR) << "Error converting wifi mac info";
                    return;
                }
                for (const auto& callback : shared_ptr_this->getEventCallbacks()) {
                    if (!callback->onRadioModeChange(aidl_radio_mode_infos).isOk()) {
                        LOG(ERROR) << "Failed to invoke onRadioModeChange callback";
                    }
                }
            };
    legacy_hal::wifi_error legacy_status =
            legacy_hal_.lock()->registerRadioModeChangeCallbackHandler(
                    getFirstActiveWlanIfaceName(), on_radio_mode_change_callback);
    return createWifiStatusFromLegacyError(legacy_status);
}

std::vector<IWifiChip::ChipConcurrencyCombination>
WifiChip::getCurrentModeConcurrencyCombinations() {
    if (!isValidModeId(current_mode_id_)) {
        LOG(ERROR) << "Chip not configured in a mode yet";
        return std::vector<IWifiChip::ChipConcurrencyCombination>();
    }
    for (const auto& mode : modes_) {
        if (mode.id == current_mode_id_) {
            return mode.availableCombinations;
        }
    }
    CHECK(0) << "Expected to find concurrency combinations for current mode!";
    return std::vector<IWifiChip::ChipConcurrencyCombination>();
}

// Returns a map indexed by IfaceConcurrencyType with the number of ifaces currently
// created of the corresponding concurrency type.
std::map<IfaceConcurrencyType, size_t> WifiChip::getCurrentConcurrencyCombination() {
    std::map<IfaceConcurrencyType, size_t> iface_counts;
    uint32_t num_ap = 0;
    uint32_t num_ap_bridged = 0;
    for (const auto& ap_iface : ap_ifaces_) {
        std::string ap_iface_name = ap_iface->getName();
        if (br_ifaces_ap_instances_.count(ap_iface_name) > 0 &&
            br_ifaces_ap_instances_[ap_iface_name].size() > 1) {
            num_ap_bridged++;
        } else {
            num_ap++;
        }
    }
    iface_counts[IfaceConcurrencyType::AP] = num_ap;
    iface_counts[IfaceConcurrencyType::AP_BRIDGED] = num_ap_bridged;
    iface_counts[IfaceConcurrencyType::NAN_IFACE] = nan_ifaces_.size();
    iface_counts[IfaceConcurrencyType::P2P] = p2p_ifaces_.size();
    iface_counts[IfaceConcurrencyType::STA] = sta_ifaces_.size();
    return iface_counts;
}

// This expands the provided concurrency combinations to a more parseable
// form. Returns a vector of available combinations possible with the number
// of each concurrency type in the combination.
// This method is a port of HalDeviceManager.expandConcurrencyCombos() from framework.
std::vector<std::map<IfaceConcurrencyType, size_t>> WifiChip::expandConcurrencyCombinations(
        const IWifiChip::ChipConcurrencyCombination& combination) {
    int32_t num_expanded_combos = 1;
    for (const auto& limit : combination.limits) {
        for (int32_t i = 0; i < limit.maxIfaces; i++) {
            num_expanded_combos *= limit.types.size();
        }
    }

    // Allocate the vector of expanded combos and reset all concurrency type counts to 0
    // in each combo.
    std::vector<std::map<IfaceConcurrencyType, size_t>> expanded_combos;
    expanded_combos.resize(num_expanded_combos);
    for (auto& expanded_combo : expanded_combos) {
        for (const auto type : {IfaceConcurrencyType::AP, IfaceConcurrencyType::AP_BRIDGED,
                                IfaceConcurrencyType::NAN_IFACE, IfaceConcurrencyType::P2P,
                                IfaceConcurrencyType::STA}) {
            expanded_combo[type] = 0;
        }
    }
    int32_t span = num_expanded_combos;
    for (const auto& limit : combination.limits) {
        for (int32_t i = 0; i < limit.maxIfaces; i++) {
            span /= limit.types.size();
            for (int32_t k = 0; k < num_expanded_combos; ++k) {
                const auto iface_type = limit.types[(k / span) % limit.types.size()];
                expanded_combos[k][iface_type]++;
            }
        }
    }
    return expanded_combos;
}

bool WifiChip::canExpandedConcurrencyComboSupportConcurrencyTypeWithCurrentTypes(
        const std::map<IfaceConcurrencyType, size_t>& expanded_combo,
        IfaceConcurrencyType requested_type) {
    const auto current_combo = getCurrentConcurrencyCombination();

    // Check if we have space for 1 more iface of |type| in this combo
    for (const auto type :
         {IfaceConcurrencyType::AP, IfaceConcurrencyType::AP_BRIDGED,
          IfaceConcurrencyType::NAN_IFACE, IfaceConcurrencyType::P2P, IfaceConcurrencyType::STA}) {
        size_t num_ifaces_needed = current_combo.at(type);
        if (type == requested_type) {
            num_ifaces_needed++;
        }
        size_t num_ifaces_allowed = expanded_combo.at(type);
        if (num_ifaces_needed > num_ifaces_allowed) {
            return false;
        }
    }
    return true;
}

// This method does the following:
// a) Enumerate all possible concurrency combos by expanding the current
//    ChipConcurrencyCombination.
// b) Check if the requested concurrency type can be added to the current mode
//    with the concurrency combination that is already active.
bool WifiChip::canCurrentModeSupportConcurrencyTypeWithCurrentTypes(
        IfaceConcurrencyType requested_type) {
    if (!isValidModeId(current_mode_id_)) {
        LOG(ERROR) << "Chip not configured in a mode yet";
        return false;
    }
    const auto combinations = getCurrentModeConcurrencyCombinations();
    for (const auto& combination : combinations) {
        const auto expanded_combos = expandConcurrencyCombinations(combination);
        for (const auto& expanded_combo : expanded_combos) {
            if (canExpandedConcurrencyComboSupportConcurrencyTypeWithCurrentTypes(expanded_combo,
                                                                                  requested_type)) {
                return true;
            }
        }
    }
    return false;
}

// Note: This does not consider concurrency types already active. It only checks if the
// provided expanded concurrency combination can support the requested combo.
bool WifiChip::canExpandedConcurrencyComboSupportConcurrencyCombo(
        const std::map<IfaceConcurrencyType, size_t>& expanded_combo,
        const std::map<IfaceConcurrencyType, size_t>& req_combo) {
    // Check if we have space for 1 more |type| in this combo
    for (const auto type :
         {IfaceConcurrencyType::AP, IfaceConcurrencyType::AP_BRIDGED,
          IfaceConcurrencyType::NAN_IFACE, IfaceConcurrencyType::P2P, IfaceConcurrencyType::STA}) {
        if (req_combo.count(type) == 0) {
            // Concurrency type not in the req_combo.
            continue;
        }
        size_t num_ifaces_needed = req_combo.at(type);
        size_t num_ifaces_allowed = expanded_combo.at(type);
        if (num_ifaces_needed > num_ifaces_allowed) {
            return false;
        }
    }
    return true;
}

// This method does the following:
// a) Enumerate all possible concurrency combos by expanding the current
//    ChipConcurrencyCombination.
// b) Check if the requested concurrency combo can be added to the current mode.
// Note: This does not consider concurrency types already active. It only checks if the
// current mode can support the requested combo.
bool WifiChip::canCurrentModeSupportConcurrencyCombo(
        const std::map<IfaceConcurrencyType, size_t>& req_combo) {
    if (!isValidModeId(current_mode_id_)) {
        LOG(ERROR) << "Chip not configured in a mode yet";
        return false;
    }
    const auto combinations = getCurrentModeConcurrencyCombinations();
    for (const auto& combination : combinations) {
        const auto expanded_combos = expandConcurrencyCombinations(combination);
        for (const auto& expanded_combo : expanded_combos) {
            if (canExpandedConcurrencyComboSupportConcurrencyCombo(expanded_combo, req_combo)) {
                return true;
            }
        }
    }
    return false;
}

// This method does the following:
// a) Enumerate all possible concurrency combos by expanding the current
//    ChipConcurrencyCombination.
// b) Check if the requested concurrency type can be added to the current mode.
bool WifiChip::canCurrentModeSupportConcurrencyType(IfaceConcurrencyType requested_type) {
    // Check if we can support at least 1 of the requested concurrency type.
    std::map<IfaceConcurrencyType, size_t> req_iface_combo;
    req_iface_combo[requested_type] = 1;
    return canCurrentModeSupportConcurrencyCombo(req_iface_combo);
}

bool WifiChip::isValidModeId(int32_t mode_id) {
    for (const auto& mode : modes_) {
        if (mode.id == mode_id) {
            return true;
        }
    }
    return false;
}

bool WifiChip::isStaApConcurrencyAllowedInCurrentMode() {
    // Check if we can support at least 1 STA & 1 AP concurrently.
    std::map<IfaceConcurrencyType, size_t> req_iface_combo;
    req_iface_combo[IfaceConcurrencyType::STA] = 1;
    req_iface_combo[IfaceConcurrencyType::AP] = 1;
    return canCurrentModeSupportConcurrencyCombo(req_iface_combo);
}

bool WifiChip::isDualStaConcurrencyAllowedInCurrentMode() {
    // Check if we can support at least 2 STA concurrently.
    std::map<IfaceConcurrencyType, size_t> req_iface_combo;
    req_iface_combo[IfaceConcurrencyType::STA] = 2;
    return canCurrentModeSupportConcurrencyCombo(req_iface_combo);
}

std::string WifiChip::getFirstActiveWlanIfaceName() {
    if (sta_ifaces_.size() > 0) return sta_ifaces_[0]->getName();
    if (ap_ifaces_.size() > 0) {
        // If the first active wlan iface is bridged iface.
        // Return first instance name.
        for (auto const& it : br_ifaces_ap_instances_) {
            if (it.first == ap_ifaces_[0]->getName()) {
                return it.second[0];
            }
        }
        return ap_ifaces_[0]->getName();
    }
    // This could happen if the chip call is made before any STA/AP
    // iface is created. Default to wlan0 for such cases.
    LOG(WARNING) << "No active wlan interfaces in use! Using default";
    return getWlanIfaceNameWithType(IfaceType::STA, 0);
}

// Return the first wlan (wlan0, wlan1 etc.) starting from |start_idx|
// not already in use.
// Note: This doesn't check the actual presence of these interfaces.
std::string WifiChip::allocateApOrStaIfaceName(IfaceType type, uint32_t start_idx) {
    for (unsigned idx = start_idx; idx < kMaxWlanIfaces; idx++) {
        const auto ifname = getWlanIfaceNameWithType(type, idx);
        if (findUsingNameFromBridgedApInstances(ifname)) continue;
        if (findUsingName(ap_ifaces_, ifname)) continue;
        if (findUsingName(sta_ifaces_, ifname)) continue;
        return ifname;
    }
    // This should never happen. We screwed up somewhere if it did.
    CHECK(false) << "All wlan interfaces in use already!";
    return {};
}

uint32_t WifiChip::startIdxOfApIface() {
    if (isDualStaConcurrencyAllowedInCurrentMode()) {
        // When the HAL support dual STAs, AP should start with idx 2.
        return 2;
    } else if (isStaApConcurrencyAllowedInCurrentMode()) {
        //  When the HAL support STA + AP but it doesn't support dual STAs.
        //  AP should start with idx 1.
        return 1;
    }
    // No concurrency support.
    return 0;
}

// AP iface names start with idx 1 for modes supporting
// concurrent STA and not dual AP, else start with idx 0.
std::string WifiChip::allocateApIfaceName() {
    // Check if we have a dedicated iface for AP.
    std::vector<std::string> ifnames = getPredefinedApIfaceNames(true);
    for (auto const& ifname : ifnames) {
        if (findUsingName(ap_ifaces_, ifname)) continue;
        return ifname;
    }
    return allocateApOrStaIfaceName(IfaceType::AP, startIdxOfApIface());
}

std::vector<std::string> WifiChip::allocateBridgedApInstanceNames() {
    // Check if we have a dedicated iface for AP.
    std::vector<std::string> instances = getPredefinedApIfaceNames(true);
    if (instances.size() == 2) {
        return instances;
    } else {
        int num_ifaces_need_to_allocate = 2 - instances.size();
        for (int i = 0; i < num_ifaces_need_to_allocate; i++) {
            std::string instance_name =
                    allocateApOrStaIfaceName(IfaceType::AP, startIdxOfApIface() + i);
            if (!instance_name.empty()) {
                instances.push_back(instance_name);
            }
        }
    }
    return instances;
}

// STA iface names start with idx 0.
// Primary STA iface will always be 0.
std::string WifiChip::allocateStaIfaceName() {
    return allocateApOrStaIfaceName(IfaceType::STA, 0);
}

bool WifiChip::writeRingbufferFilesInternal() {
    if (!removeOldFilesInternal()) {
        LOG(ERROR) << "Error occurred while deleting old tombstone files";
        return false;
    }
    // write ringbuffers to file
    {
        std::unique_lock<std::mutex> lk(lock_t);
        for (auto& item : ringbuffer_map_) {
            Ringbuffer& cur_buffer = item.second;
            if (cur_buffer.getData().empty()) {
                continue;
            }
            const std::string file_path_raw = kTombstoneFolderPath + item.first + "XXXXXXXXXX";
            const int dump_fd = mkstemp(makeCharVec(file_path_raw).data());
            if (dump_fd == -1) {
                PLOG(ERROR) << "create file failed";
                return false;
            }
            unique_fd file_auto_closer(dump_fd);
            for (const auto& cur_block : cur_buffer.getData()) {
                if (cur_block.size() <= 0 || cur_block.size() > kMaxBufferSizeBytes) {
                    PLOG(ERROR) << "Ring buffer: " << item.first
                                << " is corrupted. Invalid block size: " << cur_block.size();
                    break;
                }
                if (write(dump_fd, cur_block.data(), sizeof(cur_block[0]) * cur_block.size()) ==
                    -1) {
                    PLOG(ERROR) << "Error writing to file";
                }
            }
            cur_buffer.clear();
        }
        // unique_lock unlocked here
    }
    return true;
}

std::string WifiChip::getWlanIfaceNameWithType(IfaceType type, unsigned idx) {
    std::string ifname;

    // let the legacy hal override the interface name
    legacy_hal::wifi_error err = legacy_hal_.lock()->getSupportedIfaceName((uint32_t)type, ifname);
    if (err == legacy_hal::WIFI_SUCCESS) return ifname;

    return getWlanIfaceName(idx);
}

void WifiChip::invalidateAndClearBridgedApAll() {
    for (auto const& it : br_ifaces_ap_instances_) {
        for (auto const& iface : it.second) {
            iface_util_->removeIfaceFromBridge(it.first, iface);
            legacy_hal_.lock()->deleteVirtualInterface(iface);
        }
        iface_util_->deleteBridge(it.first);
    }
    br_ifaces_ap_instances_.clear();
}

void WifiChip::deleteApIface(const std::string& if_name) {
    if (if_name.empty()) return;
    // delete bridged interfaces if any
    for (auto const& it : br_ifaces_ap_instances_) {
        if (it.first == if_name) {
            for (auto const& iface : it.second) {
                iface_util_->removeIfaceFromBridge(if_name, iface);
                legacy_hal_.lock()->deleteVirtualInterface(iface);
            }
            iface_util_->deleteBridge(if_name);
            br_ifaces_ap_instances_.erase(if_name);
            // ifname is bridged AP, return here.
            return;
        }
    }

    // No bridged AP case, delete AP iface
    legacy_hal::wifi_error legacy_status = legacy_hal_.lock()->deleteVirtualInterface(if_name);
    if (legacy_status != legacy_hal::WIFI_SUCCESS) {
        LOG(ERROR) << "Failed to remove interface: " << if_name << " "
                   << legacyErrorToString(legacy_status);
    }
}

bool WifiChip::findUsingNameFromBridgedApInstances(const std::string& name) {
    for (auto const& it : br_ifaces_ap_instances_) {
        if (it.first == name) {
            return true;
        }
        for (auto const& iface : it.second) {
            if (iface == name) {
                return true;
            }
        }
    }
    return false;
}

ndk::ScopedAStatus WifiChip::setMloModeInternal(const WifiChip::ChipMloMode in_mode) {
    legacy_hal::wifi_mlo_mode mode;
    switch (in_mode) {
        case WifiChip::ChipMloMode::DEFAULT:
            mode = legacy_hal::wifi_mlo_mode::WIFI_MLO_MODE_DEFAULT;
            break;
        case WifiChip::ChipMloMode::LOW_LATENCY:
            mode = legacy_hal::wifi_mlo_mode::WIFI_MLO_MODE_LOW_LATENCY;
            break;
        case WifiChip::ChipMloMode::HIGH_THROUGHPUT:
            mode = legacy_hal::wifi_mlo_mode::WIFI_MLO_MODE_HIGH_THROUGHPUT;
            break;
        case WifiChip::ChipMloMode::LOW_POWER:
            mode = legacy_hal::wifi_mlo_mode::WIFI_MLO_MODE_LOW_POWER;
            break;
        default:
            PLOG(ERROR) << "Error: invalid mode: " << toString(in_mode);
            return createWifiStatus(WifiStatusCode::ERROR_INVALID_ARGS);
    }
    return createWifiStatusFromLegacyError(legacy_hal_.lock()->setMloMode(mode));
}

}  // namespace wifi
}  // namespace hardware
}  // namespace android
}  // namespace aidl
