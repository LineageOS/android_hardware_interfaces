/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include <fcntl.h>
#include <unistd.h>

#include <algorithm>

#include <HidlUtils.h>
#include <system/audio.h>
#include <system/audio_config.h>

#include "DeviceManager.h"
#include "PolicyConfig.h"
#include "common/all-versions/HidlSupport.h"

using ::android::NO_ERROR;
using ::android::OK;

using namespace ::android::hardware::audio::common::CPP_VERSION;
using namespace ::android::hardware::audio::CPP_VERSION;
using ::android::hardware::audio::common::CPP_VERSION::implementation::HidlUtils;
using ::android::hardware::audio::common::utils::splitString;
namespace xsd {
using namespace ::android::audio::policy::configuration::CPP_VERSION;
using Module = Modules::Module;
}  // namespace xsd

std::string PolicyConfig::getError() const {
    if (mFilePath.empty()) {
        return "Could not find " + mConfigFileName +
               " file in: " + testing::PrintToString(android::audio_get_configuration_paths());
    } else {
        return "Invalid config file: " + mFilePath;
    }
}

const xsd::Module* PolicyConfig::getModuleFromName(const std::string& name) const {
    if (mConfig && mConfig->getFirstModules()) {
        for (const auto& module : mConfig->getFirstModules()->get_module()) {
            if (module.getName() == name) return &module;
        }
    }
    return nullptr;
}

std::optional<DeviceAddress> PolicyConfig::getSinkDeviceForMixPort(
        const std::string& moduleName, const std::string& mixPortName) const {
    std::string device;
    if (auto module = getModuleFromName(moduleName); module) {
        auto possibleDevices = getSinkDevicesForMixPort(moduleName, mixPortName);
        if (module->hasDefaultOutputDevice() &&
            possibleDevices.count(module->getDefaultOutputDevice())) {
            device = module->getDefaultOutputDevice();
        } else {
            device = getAttachedSinkDeviceForMixPort(moduleName, mixPortName);
        }
    }
    if (!device.empty()) {
        return getDeviceAddressOfDevicePort(moduleName, device);
    }
    ALOGE("Could not find a route for the mix port \"%s\" in module \"%s\"", mixPortName.c_str(),
          moduleName.c_str());
    return std::optional<DeviceAddress>{};
}

std::optional<DeviceAddress> PolicyConfig::getSourceDeviceForMixPort(
        const std::string& moduleName, const std::string& mixPortName) const {
    const std::string device = getAttachedSourceDeviceForMixPort(moduleName, mixPortName);
    if (!device.empty()) {
        return getDeviceAddressOfDevicePort(moduleName, device);
    }
    ALOGE("Could not find a route for the mix port \"%s\" in module \"%s\"", mixPortName.c_str(),
          moduleName.c_str());
    return std::optional<DeviceAddress>{};
}

bool PolicyConfig::haveInputProfilesInModule(const std::string& name) const {
    auto module = getModuleFromName(name);
    if (module && module->getFirstMixPorts()) {
        for (const auto& mixPort : module->getFirstMixPorts()->getMixPort()) {
            if (mixPort.getRole() == xsd::Role::sink) return true;
        }
    }
    return false;
}

// static
std::string PolicyConfig::findExistingConfigurationFile(const std::string& fileName) {
    for (const auto& location : android::audio_get_configuration_paths()) {
        std::string path = location + '/' + fileName;
        if (access(path.c_str(), F_OK) == 0) {
            return path;
        }
    }
    return {};
}

std::string PolicyConfig::findAttachedDevice(const std::vector<std::string>& attachedDevices,
                                             const std::set<std::string>& possibleDevices) const {
    for (const auto& device : attachedDevices) {
        if (possibleDevices.count(device)) return device;
    }
    return {};
}

const std::vector<std::string>& PolicyConfig::getAttachedDevices(
        const std::string& moduleName) const {
    static const std::vector<std::string> empty;
    auto module = getModuleFromName(moduleName);
    if (module && module->getFirstAttachedDevices()) {
        return module->getFirstAttachedDevices()->getItem();
    }
    return empty;
}

std::optional<DeviceAddress> PolicyConfig::getDeviceAddressOfDevicePort(
        const std::string& moduleName, const std::string& devicePortName) const {
    auto module = getModuleFromName(moduleName);
    if (module->getFirstDevicePorts()) {
        const auto& devicePorts = module->getFirstDevicePorts()->getDevicePort();
        const auto& devicePort = std::find_if(
                devicePorts.begin(), devicePorts.end(),
                [&devicePortName](auto dp) { return dp.getTagName() == devicePortName; });
        if (devicePort != devicePorts.end()) {
            audio_devices_t halDeviceType;
            if (HidlUtils::audioDeviceTypeToHal(devicePort->getType(), &halDeviceType) ==
                NO_ERROR) {
                // For AOSP device types use the standard parser for the device address.
                const std::string address =
                        devicePort->hasAddress() ? devicePort->getAddress() : "";
                DeviceAddress result;
                if (HidlUtils::deviceAddressFromHal(halDeviceType, address.c_str(), &result) ==
                    NO_ERROR) {
                    return result;
                }
            } else if (xsd::isVendorExtension(devicePort->getType())) {
                DeviceAddress result;
                result.deviceType = devicePort->getType();
                if (devicePort->hasAddress()) {
                    result.address.id(devicePort->getAddress());
                }
                return result;
            }
        } else {
            ALOGE("Device port \"%s\" not found in module \"%s\"", devicePortName.c_str(),
                  moduleName.c_str());
        }
    } else {
        ALOGE("Module \"%s\" has no device ports", moduleName.c_str());
    }
    return std::optional<DeviceAddress>{};
}

std::set<std::string> PolicyConfig::getSinkDevicesForMixPort(const std::string& moduleName,
                                                             const std::string& mixPortName) const {
    std::set<std::string> result;
    auto module = getModuleFromName(moduleName);
    if (module && module->getFirstRoutes()) {
        for (const auto& route : module->getFirstRoutes()->getRoute()) {
            const auto sources = splitString(route.getSources(), ',');
            if (std::find(sources.begin(), sources.end(), mixPortName) != sources.end()) {
                result.insert(route.getSink());
            }
        }
    }
    return result;
}

std::set<std::string> PolicyConfig::getSourceDevicesForMixPort(
        const std::string& moduleName, const std::string& mixPortName) const {
    std::set<std::string> result;
    auto module = getModuleFromName(moduleName);
    if (module && module->getFirstRoutes()) {
        const auto& routes = module->getFirstRoutes()->getRoute();
        const auto route = std::find_if(routes.begin(), routes.end(), [&mixPortName](auto rte) {
            return rte.getSink() == mixPortName;
        });
        if (route != routes.end()) {
            const auto sources = splitString(route->getSources(), ',');
            std::copy(sources.begin(), sources.end(), std::inserter(result, result.end()));
        }
    }
    return result;
}

void PolicyConfig::init() {
    if (mConfig) {
        mStatus = OK;
        mPrimaryModule = getModuleFromName(DeviceManager::kPrimaryDevice);
        if (mConfig->getFirstModules()) {
            for (const auto& module : mConfig->getFirstModules()->get_module()) {
                if (module.getFirstAttachedDevices()) {
                    auto attachedDevices = module.getFirstAttachedDevices()->getItem();
                    if (!attachedDevices.empty()) {
                        mModulesWithDevicesNames.insert(module.getName());
                    }
                }
            }
        }
    }
}
