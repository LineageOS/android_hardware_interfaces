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

#pragma once

#include <set>
#include <string>

#include <DeviceDescriptor.h>
#include <HwModule.h>
#include <Serializer.h>
#include <gtest/gtest.h>
#include <system/audio_config.h>

#include "DeviceManager.h"

using ::android::sp;
using ::android::status_t;

struct PolicyConfigData {
    android::HwModuleCollection hwModules;
    android::DeviceVector availableOutputDevices;
    android::DeviceVector availableInputDevices;
    sp<android::DeviceDescriptor> defaultOutputDevice;
};

class PolicyConfig : private PolicyConfigData, public android::AudioPolicyConfig {
  public:
    explicit PolicyConfig(const std::string& configFileName)
        : android::AudioPolicyConfig(hwModules, availableOutputDevices, availableInputDevices,
                                     defaultOutputDevice),
          mConfigFileName{configFileName} {
        for (const auto& location : android::audio_get_configuration_paths()) {
            std::string path = location + '/' + mConfigFileName;
            if (access(path.c_str(), F_OK) == 0) {
                mFilePath = path;
                break;
            }
        }
        init();
    }
    PolicyConfig(const std::string& configPath, const std::string& configFileName)
        : android::AudioPolicyConfig(hwModules, availableOutputDevices, availableInputDevices,
                                     defaultOutputDevice),
          mConfigFileName{configFileName},
          mFilePath{configPath + "/" + mConfigFileName} {
        init();
    }
    status_t getStatus() const { return mStatus; }
    std::string getError() const {
        if (mFilePath.empty()) {
            return std::string{"Could not find "} + mConfigFileName +
                   " file in: " + testing::PrintToString(android::audio_get_configuration_paths());
        } else {
            return "Invalid config file: " + mFilePath;
        }
    }
    const std::string& getFilePath() const { return mFilePath; }
    sp<const android::HwModule> getModuleFromName(const std::string& name) const {
        return getHwModules().getModuleFromName(name.c_str());
    }
    sp<const android::HwModule> getPrimaryModule() const { return mPrimaryModule; }
    const std::set<std::string>& getModulesWithDevicesNames() const {
        return mModulesWithDevicesNames;
    }
    std::string getAttachedSinkDeviceForMixPort(const std::string& moduleName,
                                                const std::string& mixPortName) const {
        return findAttachedDevice(getAttachedDevices(moduleName),
                                  getSinkDevicesForMixPort(moduleName, mixPortName));
    }
    std::string getAttachedSourceDeviceForMixPort(const std::string& moduleName,
                                                  const std::string& mixPortName) const {
        return findAttachedDevice(getAttachedDevices(moduleName),
                                  getSourceDevicesForMixPort(moduleName, mixPortName));
    }
    bool haveInputProfilesInModule(const std::string& name) const {
        auto module = getModuleFromName(name);
        return module && !module->getInputProfiles().empty();
    }

  private:
    void init() {
        mStatus = android::deserializeAudioPolicyFileForVts(mFilePath.c_str(), this);
        if (mStatus == android::OK) {
            mPrimaryModule = getModuleFromName(DeviceManager::kPrimaryDevice);
            // Available devices are not 'attached' to modules at this moment.
            // Need to go over available devices and find their module.
            for (const auto& device : availableOutputDevices) {
                for (const auto& module : hwModules) {
                    if (module->getDeclaredDevices().indexOf(device) >= 0) {
                        mModulesWithDevicesNames.insert(module->getName());
                        mAttachedDevicesPerModule[module->getName()].push_back(
                                device->getTagName());
                        break;
                    }
                }
            }
            for (const auto& device : availableInputDevices) {
                for (const auto& module : hwModules) {
                    if (module->getDeclaredDevices().indexOf(device) >= 0) {
                        mModulesWithDevicesNames.insert(module->getName());
                        mAttachedDevicesPerModule[module->getName()].push_back(
                                device->getTagName());
                        break;
                    }
                }
            }
        }
    }
    std::string findAttachedDevice(const std::vector<std::string>& attachedDevices,
                                   const std::set<std::string>& possibleDevices) const {
        for (const auto& device : attachedDevices) {
            if (possibleDevices.count(device)) return device;
        }
        return {};
    }
    std::vector<std::string> getAttachedDevices(const std::string& moduleName) const {
        if (auto iter = mAttachedDevicesPerModule.find(moduleName);
            iter != mAttachedDevicesPerModule.end()) {
            return iter->second;
        }
        return {};
    }
    std::set<std::string> getSinkDevicesForMixPort(const std::string& moduleName,
                                                   const std::string& mixPortName) const {
        std::set<std::string> result;
        auto module = getModuleFromName(moduleName);
        if (module != nullptr) {
            for (const auto& route : module->getRoutes()) {
                for (const auto& source : route->getSources()) {
                    if (source->getTagName() == mixPortName) {
                        result.insert(route->getSink()->getTagName());
                    }
                }
            }
        }
        return result;
    }
    std::set<std::string> getSourceDevicesForMixPort(const std::string& moduleName,
                                                     const std::string& mixPortName) const {
        std::set<std::string> result;
        auto module = getModuleFromName(moduleName);
        if (module != nullptr) {
            for (const auto& route : module->getRoutes()) {
                if (route->getSink()->getTagName() == mixPortName) {
                    const auto& sources = route->getSources();
                    std::transform(sources.begin(), sources.end(),
                                   std::inserter(result, result.end()),
                                   [](const auto& source) { return source->getTagName(); });
                }
            }
        }
        return result;
    }

    const std::string mConfigFileName;
    status_t mStatus = android::NO_INIT;
    std::string mFilePath;
    sp<const android::HwModule> mPrimaryModule = nullptr;
    std::set<std::string> mModulesWithDevicesNames;
    std::map<std::string, std::vector<std::string>> mAttachedDevicesPerModule;
};
