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

#include "canbus_config.pb.h"
#include "canprototools.h"

#include <aidl/android/hardware/automotive/can/ICanController.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>

#include <chrono>
#include <thread>

namespace android::hardware::automotive::can {

using namespace std::string_literals;
using ::aidl::android::hardware::automotive::can::ICanController;

static constexpr std::string_view kDefaultConfigPath = "/etc/canbus_config.pb";

/**
 * Takes output from parsed protobuf config and uses it to configure the CAN HAL.
 *
 * \param pb_cfg is an instance of the autogenerated protobuf object for our configuration.
 * \return boolean status, true on success, false on failure.
 */
static bool processPbCfg(const config::CanBusConfig& pb_cfg) {
    for (auto const& bus : pb_cfg.buses()) {
        if (bus.name().empty()) {
            LOG(ERROR) << "Invalid config: Bus config must have a valid name field";
            return false;
        }

        auto busCfgMaybe = config::fromPbBus(bus);
        if (!busCfgMaybe.has_value()) {
            return false;
        }
        auto busCfg = *busCfgMaybe;

        const auto instance = ICanController::descriptor + "/default"s;
        const auto service = ICanController::fromBinder(
                ndk::SpAIBinder(AServiceManager_waitForService(instance.c_str())));
        if (service == nullptr) {
            LOG(FATAL) << "Can't find CAN HAL! (has it started yet?)";
            return false;
        }

        LOG(VERBOSE) << "Bringing up a " << busCfg.name << " @ " << busCfg.bitrate;

        std::string ifaceName;
        const auto status = service->upBus(busCfg, &ifaceName);
        if (!status.isOk() && status.getExceptionCode() != EX_SERVICE_SPECIFIC) {
            LOG(FATAL) << "Binder transaction failed!" << status.getStatus();
            return false;
        } else if (!status.isOk()) {
            LOG(ERROR) << "upBus failed: " << config::resultStringFromStatus(status) << ": "
                       << status.getMessage();
            continue;
        }

        LOG(INFO) << bus.name() << " has been successfully configured on " << ifaceName;
    }
    return true;
}

/**
 * This kicks off the CAN HAL configuration process. This starts the following:
 *     1. Reading the config file
 *     2. Setting up CAN buses
 *     3. Handling services
 * \param filepath is a string specifying the absolute path of the config file
 * \return boolean status, true on success, false on failure
 */
static bool configuratorStart(const std::string& filepath) {
    base::SetDefaultTag("CanConfigurator");
    auto pbCfg = config::parseConfigFile(filepath);
    if (!pbCfg.has_value()) {
        return false;
    }
    // process the rest of the config file data and configure the CAN buses.
    if (!processPbCfg(*pbCfg)) {
        return false;
    }
    LOG(INFO) << "CAN HAL has been configured!";
    return true;
}

extern "C" int main(int argc, char* argv[]) {
    std::string configFilepath = static_cast<std::string>(kDefaultConfigPath);

    // allow for CLI specification of a config file.
    if (argc == 2) {
        configFilepath = argv[1];
    } else if (argc > 2) {
        std::cerr << "usage: " << argv[0] << " [optional config filepath]";
        return 1;
    }

    if (!configuratorStart(configFilepath)) {
        return 1;
    }
    return 0;
}

}  // namespace android::hardware::automotive::can