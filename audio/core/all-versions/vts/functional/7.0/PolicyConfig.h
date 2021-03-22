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

#include <optional>
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <utils/Errors.h>

// clang-format off
#include PATH(android/hardware/audio/FILE_VERSION/types.h)
#include PATH(android/hardware/audio/common/FILE_VERSION/types.h)
// clang-format on

#include <android_audio_policy_configuration_V7_0-enums.h>
#include <android_audio_policy_configuration_V7_0.h>

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
    android::status_t getStatus() const { return mStatus; }
    std::string getError() const;
    const std::string& getFilePath() const { return mFilePath; }
    const xsd::Module* getModuleFromName(const std::string& name) const;
    const xsd::Module* getPrimaryModule() const { return mPrimaryModule; }
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
    std::optional<DeviceAddress> getSinkDeviceForMixPort(const std::string& moduleName,
                                                         const std::string& mixPortName) const;
    std::optional<DeviceAddress> getSourceDeviceForMixPort(const std::string& moduleName,
                                                           const std::string& mixPortName) const;
    bool haveInputProfilesInModule(const std::string& name) const;

  private:
    static std::string findExistingConfigurationFile(const std::string& fileName);
    std::string findAttachedDevice(const std::vector<std::string>& attachedDevices,
                                   const std::set<std::string>& possibleDevices) const;
    const std::vector<std::string>& getAttachedDevices(const std::string& moduleName) const;
    std::optional<DeviceAddress> getDeviceAddressOfDevicePort(
            const std::string& moduleName, const std::string& devicePortName) const;
    std::string getDevicePortTagNameFromType(const std::string& moduleName,
                                             const AudioDevice& deviceType) const;
    std::set<std::string> getSinkDevicesForMixPort(const std::string& moduleName,
                                                   const std::string& mixPortName) const;
    std::set<std::string> getSourceDevicesForMixPort(const std::string& moduleName,
                                                     const std::string& mixPortName) const;
    void init();

    const std::string mConfigFileName;
    const std::string mFilePath;
    std::optional<xsd::AudioPolicyConfiguration> mConfig;
    android::status_t mStatus = android::NO_INIT;
    const xsd::Module* mPrimaryModule;
    std::set<std::string> mModulesWithDevicesNames;
};
