/*
 ** Copyright 2018, The Android Open Source Project
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#include <keymasterV4_1/Keymaster.h>

#include <iomanip>

#include <android-base/logging.h>
#include <android/hidl/manager/1.2/IServiceManager.h>
#include <keymasterV4_0/key_param_output.h>
#include <keymasterV4_0/keymaster_utils.h>
#include <keymasterV4_1/Keymaster3.h>
#include <keymasterV4_1/Keymaster4.h>

namespace android::hardware {

template <class T>
std::ostream& operator<<(std::ostream& os, const hidl_vec<T>& vec) {
    os << "{ ";
    if (vec.size()) {
        for (size_t i = 0; i < vec.size() - 1; ++i) os << vec[i] << ", ";
        os << vec[vec.size() - 1];
    }
    os << " }";
    return os;
}

std::ostream& operator<<(std::ostream& os, const hidl_vec<uint8_t>& vec) {
    std::ios_base::fmtflags flags(os.flags());
    os << std::setw(2) << std::setfill('0') << std::hex;
    for (uint8_t c : vec) os << static_cast<int>(c);
    os.flags(flags);
    return os;
}

template <size_t N>
std::ostream& operator<<(std::ostream& os, const hidl_array<uint8_t, N>& vec) {
    std::ios_base::fmtflags flags(os.flags());
    os << std::setw(2) << std::setfill('0') << std::hex;
    for (size_t i = 0; i < N; ++i) os << static_cast<int>(vec[i]);
    os.flags(flags);
    return os;
}

namespace keymaster {

namespace V4_0 {

std::ostream& operator<<(std::ostream& os, const HmacSharingParameters& params) {
    // Note that by design, although seed and nonce are used to compute a secret, they are
    // not secrets and it's just fine to log them.
    os << "(seed: " << params.seed << ", nonce: " << params.nonce << ')';
    return os;
}

}  // namespace V4_0

namespace V4_1::support {

using ::android::sp;
using ::android::hidl::manager::V1_2::IServiceManager;

std::ostream& operator<<(std::ostream& os, const Keymaster& keymaster) {
    auto& version = keymaster.halVersion();
    os << version.keymasterName << " from " << version.authorName
       << " SecurityLevel: " << toString(version.securityLevel)
       << " HAL: " << keymaster.descriptor() << "/" << keymaster.instanceName();
    return os;
}

template <typename Wrapper>
Keymaster::KeymasterSet enumerateDevices(const sp<IServiceManager>& serviceManager) {
    Keymaster::KeymasterSet result;

    bool foundDefault = false;
    auto& descriptor = Wrapper::WrappedIKeymasterDevice::descriptor;
    serviceManager->listManifestByInterface(descriptor, [&](const hidl_vec<hidl_string>& names) {
        for (auto& name : names) {
            if (name == "default") foundDefault = true;
            auto device = Wrapper::WrappedIKeymasterDevice::getService(name);
            CHECK(device) << "Failed to get service for " << descriptor << " with interface name "
                          << name;
            result.push_back(new Wrapper(device, name));
        }
    });

    if (!foundDefault) {
        // "default" wasn't provided by listManifestByInterface.  Maybe there's a passthrough
        // implementation.
        auto device = Wrapper::WrappedIKeymasterDevice::getService("default");
        if (device) result.push_back(new Wrapper(device, "default"));
    }

    return result;
}

void Keymaster::logIfKeymasterVendorError(ErrorCode ec) const {
    static constexpr int32_t k_keymaster_vendor_error_code_range_max = -10000;
    if (static_cast<int32_t>(ec) <= k_keymaster_vendor_error_code_range_max) {
        const auto& versionInfo = halVersion();
        LOG(ERROR) << "Keymaster reported error: " << static_cast<int32_t>(ec) << "\n"
                   << "NOTE: This is an error in the vendor specific error range.\n"
                   << "      Refer to the vendor of the implementation for details.\n"
                   << "      Implementation name: " << versionInfo.keymasterName << "\n"
                   << "      Vendor name:         " << versionInfo.authorName << "\n"
                   << "      MajorVersion:        " << versionInfo.majorVersion;
    }
}

Keymaster::KeymasterSet Keymaster::enumerateAvailableDevices() {
    auto serviceManager = IServiceManager::getService();
    CHECK(serviceManager) << "Could not retrieve ServiceManager";

    auto km4s = enumerateDevices<Keymaster4>(serviceManager);
    auto km3s = enumerateDevices<Keymaster3>(serviceManager);

    auto result = std::move(km4s);
    result.insert(result.end(), std::make_move_iterator(km3s.begin()),
                  std::make_move_iterator(km3s.end()));

    std::sort(result.begin(), result.end(),
              [](auto& a, auto& b) { return a->halVersion() > b->halVersion(); });

    size_t i = 1;
    LOG(INFO) << "List of Keymaster HALs found:";
    for (auto& hal : result) LOG(INFO) << "Keymaster HAL #" << i++ << ": " << *hal;

    return result;
}

static hidl_vec<HmacSharingParameters> getHmacParameters(
        const Keymaster::KeymasterSet& keymasters) {
    std::vector<HmacSharingParameters> params_vec;
    params_vec.reserve(keymasters.size());
    for (auto& keymaster : keymasters) {
        if (keymaster->halVersion().majorVersion < 4) continue;
        auto rc = keymaster->getHmacSharingParameters([&](auto error, auto& params) {
            CHECK(error == V4_0::ErrorCode::OK)
                    << "Failed to get HMAC parameters from " << *keymaster << " error " << error;
            params_vec.push_back(params);
        });
        CHECK(rc.isOk()) << "Failed to communicate with " << *keymaster
                         << " error: " << rc.description();
    }
    std::sort(params_vec.begin(), params_vec.end());

    return params_vec;
}

static void computeHmac(const Keymaster::KeymasterSet& keymasters,
                        const hidl_vec<HmacSharingParameters>& params) {
    if (!params.size()) return;

    hidl_vec<uint8_t> sharingCheck;
    bool firstKeymaster = true;
    LOG(DEBUG) << "Computing HMAC with params " << params;
    for (auto& keymaster : keymasters) {
        if (keymaster->halVersion().majorVersion < 4) continue;
        LOG(DEBUG) << "Computing HMAC for " << *keymaster;
        auto rc = keymaster->computeSharedHmac(
                params, [&](V4_0::ErrorCode error, const hidl_vec<uint8_t>& curSharingCheck) {
                    CHECK(error == V4_0::ErrorCode::OK) << "Failed to get HMAC parameters from "
                                                        << *keymaster << " error " << error;
                    if (firstKeymaster) {
                        sharingCheck = curSharingCheck;
                        firstKeymaster = false;
                    }
                    if (curSharingCheck != sharingCheck)
                        LOG(WARNING) << "HMAC computation failed for " << *keymaster  //
                                     << " Expected: " << sharingCheck                 //
                                     << " got: " << curSharingCheck;
                });
        CHECK(rc.isOk()) << "Failed to communicate with " << *keymaster
                         << " error: " << rc.description();
    }
}

void Keymaster::performHmacKeyAgreement(const KeymasterSet& keymasters) {
    computeHmac(keymasters, getHmacParameters(keymasters));
}

}  // namespace V4_1::support
}  // namespace keymaster
}  // namespace android::hardware
