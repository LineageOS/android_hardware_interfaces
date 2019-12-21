/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include "CanController.h"

#include "CanBusNative.h"
#include "CanBusSlcan.h"
#include "CanBusVirtual.h"

#include <android-base/logging.h>
#include <android/hidl/manager/1.2/IServiceManager.h>

#include <regex>

namespace android::hardware::automotive::can::V1_0::implementation {

using IfaceIdDisc = ICanController::BusConfiguration::InterfaceIdentifier::hidl_discriminator;

Return<void> CanController::getSupportedInterfaceTypes(getSupportedInterfaceTypes_cb _hidl_cb) {
    _hidl_cb({ICanController::InterfaceType::VIRTUAL, ICanController::InterfaceType::SOCKETCAN,
              ICanController::InterfaceType::SLCAN});
    return {};
}

static bool isValidName(const std::string& name) {
    static const std::regex nameRE("^[a-zA-Z0-9_]{1,32}$");
    return std::regex_match(name, nameRE);
}

Return<ICanController::Result> CanController::upInterface(
        const ICanController::BusConfiguration& config) {
    LOG(VERBOSE) << "Attempting to bring interface up: " << toString(config);

    std::lock_guard<std::mutex> lck(mCanBusesGuard);

    if (!isValidName(config.name)) {
        LOG(ERROR) << "Bus name " << config.name << " is invalid";
        return ICanController::Result::UNKNOWN_ERROR;
    }

    if (mCanBuses.find(config.name) != mCanBuses.end()) {
        LOG(ERROR) << "Bus " << config.name << " is already up";
        return ICanController::Result::INVALID_STATE;
    }

    sp<CanBus> busService;

    if (config.iftype == ICanController::InterfaceType::SOCKETCAN) {
        // TODO(b/135918744): support serialno
        if (config.interfaceId.getDiscriminator() == IfaceIdDisc::address) {
            busService = new CanBusNative(config.interfaceId.address(), config.baudrate);
        } else {
            return ICanController::Result::BAD_ADDRESS;
        }
    } else if (config.iftype == ICanController::InterfaceType::VIRTUAL) {
        if (config.interfaceId.getDiscriminator() == IfaceIdDisc::address) {
            busService = new CanBusVirtual(config.interfaceId.address());
        } else {
            return ICanController::Result::BAD_ADDRESS;
        }
    } else if (config.iftype == ICanController::InterfaceType::SLCAN) {
        if (config.interfaceId.getDiscriminator() == IfaceIdDisc::address) {
            busService = new CanBusSlcan(config.interfaceId.address(), config.baudrate);
        } else {
            return ICanController::Result::BAD_ADDRESS;
        }
    } else {
        return ICanController::Result::NOT_SUPPORTED;
    }

    busService->setErrorCallback([this, name = config.name]() { downInterface(name); });

    const auto result = busService->up();
    if (result != ICanController::Result::OK) return result;

    if (busService->registerAsService(config.name) != OK) {
        LOG(ERROR) << "Failed to register ICanBus/" << config.name;
        if (!busService->down()) {
            LOG(WARNING) << "Failed to bring down CAN bus that failed to register";
        }
        return ICanController::Result::UNKNOWN_ERROR;
    }

    mCanBuses[config.name] = busService;

    return ICanController::Result::OK;
}

static bool unregisterCanBusService(const hidl_string& name, sp<CanBus> busService) {
    auto manager = hidl::manager::V1_2::IServiceManager::getService();
    if (!manager) return false;
    const auto res = manager->tryUnregister(ICanBus::descriptor, name, busService);
    if (!res.isOk()) return false;
    return res;
}

Return<bool> CanController::downInterface(const hidl_string& name) {
    LOG(VERBOSE) << "Attempting to bring interface down: " << name;

    std::lock_guard<std::mutex> lck(mCanBusesGuard);

    auto busEntry = mCanBuses.extract(name);
    if (!busEntry) {
        LOG(WARNING) << "Interface " << name << " is not up";
        return false;
    }

    auto success = true;

    if (!unregisterCanBusService(name, busEntry.mapped())) {
        LOG(ERROR) << "Couldn't unregister " << name;
        // don't return yet, let's try to do best-effort cleanup
        success = false;
    }

    if (!busEntry.mapped()->down()) {
        LOG(ERROR) << "Couldn't bring " << name << " down";
        success = false;
    }

    return success;
}

}  // namespace android::hardware::automotive::can::V1_0::implementation
