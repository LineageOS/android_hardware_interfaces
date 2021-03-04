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

#include <fcntl.h>
#include <unistd.h>

#include <optional>
#include <set>
#include <string>

#include <gtest/gtest.h>
#include <system/audio_config.h>
#include <utils/Errors.h>

// clang-format off
#include PATH(android/hardware/audio/FILE_VERSION/types.h)
#include PATH(android/hardware/audio/common/FILE_VERSION/types.h)
// clang-format on

#include <android_audio_policy_configuration_V7_0-enums.h>
#include <android_audio_policy_configuration_V7_0.h>

#include "DeviceManager.h"

using ::android::NO_INIT;
using ::android::OK;
using ::android::status_t;

using namespace ::android::hardware::audio::common::CPP_VERSION;
using namespace ::android::hardware::audio::CPP_VERSION;
namespace xsd {
using namespace ::android::audio::policy::configuration::CPP_VERSION;
using Module = Modules::Module;
}

class PolicyConfig {
  public:
    explicit PolicyConfig(const std::string& configFileName)
        : mConfigFileName{configFileName},
          mFilePath{findExistingConfigurationFile(mConfigFileName)},
          mConfig{xsd::read(mFilePath.c_str())} {
        init();
    }
    PolicyConfig(const std::string& configPath, const std::string& configFileName)
        : mConfigFileName{configFileName},
          mFilePath{configPath + "/" + mConfigFileName},
          mConfig{xsd::read(mFilePath.c_str())} {
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
    const xsd::Module* getModuleFromName(const std::string& name) const {
        if (mConfig && mConfig->getFirstModules()) {
            for (const auto& module : mConfig->getFirstModules()->get_module()) {
                if (module.getName() == name) return &module;
            }
        }
        return nullptr;
    }
    const xsd::Module* getPrimaryModule() const { return mPrimaryModule; }
    const std::set<std::string>& getModulesWithDevicesNames() const {
        return mModulesWithDevicesNames;
    }
    bool haveInputProfilesInModule(const std::string& name) const {
        auto module = getModuleFromName(name);
        if (module && module->getFirstMixPorts()) {
            for (const auto& mixPort : module->getFirstMixPorts()->getMixPort()) {
                if (mixPort.getRole() == xsd::Role::sink) return true;
            }
        }
        return false;
    }

  private:
    static std::string findExistingConfigurationFile(const std::string& fileName) {
        for (const auto& location : android::audio_get_configuration_paths()) {
            std::string path = location + '/' + fileName;
            if (access(path.c_str(), F_OK) == 0) {
                return path;
            }
        }
        return std::string{};
    }
    void init() {
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

    const std::string mConfigFileName;
    const std::string mFilePath;
    std::optional<xsd::AudioPolicyConfiguration> mConfig;
    status_t mStatus = NO_INIT;
    const xsd::Module* mPrimaryModule;
    std::set<std::string> mModulesWithDevicesNames;
};
