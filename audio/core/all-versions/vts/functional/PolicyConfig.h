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
                        break;
                    }
                }
            }
            for (const auto& device : availableInputDevices) {
                for (const auto& module : hwModules) {
                    if (module->getDeclaredDevices().indexOf(device) >= 0) {
                        mModulesWithDevicesNames.insert(module->getName());
                        break;
                    }
                }
            }
        }
    }

    const std::string mConfigFileName;
    status_t mStatus = android::NO_INIT;
    std::string mFilePath;
    sp<const android::HwModule> mPrimaryModule = nullptr;
    std::set<std::string> mModulesWithDevicesNames;
};
