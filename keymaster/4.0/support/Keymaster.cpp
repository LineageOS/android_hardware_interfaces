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

#include <keymasterV4_0/Keymaster.h>

#include <android-base/logging.h>
#include <android/hidl/manager/1.0/IServiceManager.h>
#include <keymasterV4_0/Keymaster3.h>
#include <keymasterV4_0/Keymaster4.h>

namespace android {
namespace hardware {
namespace keymaster {
namespace V4_0 {
namespace support {

using ::android::sp;
using ::android::hidl::manager::V1_0::IServiceManager;

template <typename Wrapper>
std::vector<std::unique_ptr<Keymaster>> enumerateDevices(
    const sp<IServiceManager>& serviceManager) {
    std::vector<std::unique_ptr<Keymaster>> result;

    bool foundDefault = false;
    auto& descriptor = Wrapper::WrappedIKeymasterDevice::descriptor;
    serviceManager->listByInterface(descriptor, [&](const hidl_vec<hidl_string>& names) {
        for (auto& name : names) {
            if (name == "default") foundDefault = true;
            auto device = Wrapper::WrappedIKeymasterDevice::getService();
            CHECK(device) << "Failed to get service for " << descriptor << " with interface name "
                          << name;
            result.push_back(std::unique_ptr<Keymaster>(new Wrapper(device, name)));
        }
    });

    if (!foundDefault) {
        // "default" wasn't provided by listByInterface.  Maybe there's a passthrough
        // implementation.
        auto device = Wrapper::WrappedIKeymasterDevice::getService("default");
        if (device) result.push_back(std::unique_ptr<Keymaster>(new Wrapper(device, "default")));
    }

    return result;
}

std::vector<std::unique_ptr<Keymaster>> Keymaster::enumerateAvailableDevices() {
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
    for (auto& hal : result) {
        auto& version = hal->halVersion();
        LOG(INFO) << "Keymaster HAL #" << i << ": " << version.keymasterName << " from "
                  << version.authorName << " SecurityLevel: " << toString(version.securityLevel)
                  << " HAL : " << hal->descriptor() << " instance " << hal->instanceName();
    }

    return result;
}

}  // namespace support
}  // namespace V4_0
}  // namespace keymaster
}  // namespace hardware
};  // namespace android
