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

#include "Service.h"

#include <AndroidVersionUtil.h>
#include <aidl/android/hardware/neuralnetworks/IDevice.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/hardware/neuralnetworks/1.0/IDevice.h>
#include <android/hardware/neuralnetworks/1.1/IDevice.h>
#include <android/hardware/neuralnetworks/1.2/IDevice.h>
#include <android/hardware/neuralnetworks/1.3/IDevice.h>
#include <android/hidl/manager/1.2/IServiceManager.h>
#include <hidl/ServiceManagement.h>
#include <nnapi/IDevice.h>
#include <nnapi/Result.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/hal/1.0/Service.h>
#include <nnapi/hal/1.1/Service.h>
#include <nnapi/hal/1.2/Service.h>
#include <nnapi/hal/1.3/Service.h>
#include <nnapi/hal/aidl/Service.h>
#include <nnapi/hal/aidl/Utils.h>

#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <vector>

namespace android::hardware::neuralnetworks::service {
namespace {

namespace aidl_hal = ::aidl::android::hardware::neuralnetworks;
using getDeviceFn = std::add_pointer_t<nn::GeneralResult<nn::SharedDevice>(const std::string&)>;

void getHidlDevicesForVersion(const std::string& descriptor, getDeviceFn getDevice,
                              std::vector<SharedDeviceAndUpdatability>* devices,
                              std::unordered_set<std::string>* registeredDevices) {
    CHECK(devices != nullptr);
    CHECK(registeredDevices != nullptr);

    const auto names = getAllHalInstanceNames(descriptor);
    for (const auto& name : names) {
        if (const auto [it, unregistered] = registeredDevices->insert(name); unregistered) {
            auto maybeDevice = getDevice(name);
            if (maybeDevice.has_value()) {
                auto device = std::move(maybeDevice).value();
                CHECK(device != nullptr);
                devices->push_back({.device = std::move(device)});
            } else {
                LOG(ERROR) << "getDevice(" << name << ") failed with " << maybeDevice.error().code
                           << ": " << maybeDevice.error().message;
            }
        }
    }
}

void getAidlDevices(std::vector<SharedDeviceAndUpdatability>* devices,
                    std::unordered_set<std::string>* registeredDevices,
                    bool includeUpdatableDrivers) {
    CHECK(devices != nullptr);
    CHECK(registeredDevices != nullptr);

    std::vector<std::string> names;
    constexpr auto callback = [](const char* serviceName, void* names) {
        static_cast<std::vector<std::string>*>(names)->emplace_back(serviceName);
    };

    // Devices with SDK level lower than 31 (Android S) don't have any AIDL drivers available, so
    // there is no need for a workaround supported on lower levels.
    if (__builtin_available(android __NNAPI_AIDL_MIN_ANDROID_API__, *)) {
        AServiceManager_forEachDeclaredInstance(aidl_hal::IDevice::descriptor,
                                                static_cast<void*>(&names), callback);
    }

    for (const auto& name : names) {
        bool isDeviceUpdatable = false;
        if (__builtin_available(android __NNAPI_AIDL_MIN_ANDROID_API__, *)) {
            const auto instance = std::string(aidl_hal::IDevice::descriptor) + '/' + name;
            isDeviceUpdatable = AServiceManager_isUpdatableViaApex(instance.c_str());
        }
        if (isDeviceUpdatable && !includeUpdatableDrivers) {
            continue;
        }
        if (const auto [it, unregistered] = registeredDevices->insert(name); unregistered) {
            auto maybeDevice = aidl_hal::utils::getDevice(name);
            if (maybeDevice.has_value()) {
                auto device = std::move(maybeDevice).value();
                CHECK(device != nullptr);
                devices->push_back(
                        {.device = std::move(device), .isDeviceUpdatable = isDeviceUpdatable});
            } else {
                LOG(ERROR) << "getDevice(" << name << ") failed with " << maybeDevice.error().code
                           << ": " << maybeDevice.error().message;
            }
        }
    }
}

}  // namespace

std::vector<SharedDeviceAndUpdatability> getDevices(bool includeUpdatableDrivers) {
    std::vector<SharedDeviceAndUpdatability> devices;
    std::unordered_set<std::string> registeredDevices;

    getAidlDevices(&devices, &registeredDevices, includeUpdatableDrivers);

    getHidlDevicesForVersion(V1_3::IDevice::descriptor, &V1_3::utils::getDevice, &devices,
                             &registeredDevices);
    getHidlDevicesForVersion(V1_2::IDevice::descriptor, &V1_2::utils::getDevice, &devices,
                             &registeredDevices);
    getHidlDevicesForVersion(V1_1::IDevice::descriptor, &V1_1::utils::getDevice, &devices,
                             &registeredDevices);
    getHidlDevicesForVersion(V1_0::IDevice::descriptor, &V1_0::utils::getDevice, &devices,
                             &registeredDevices);

    return devices;
}

}  // namespace android::hardware::neuralnetworks::service
