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

#include "libcanhaltools/libcanhaltools.h"

#include <android-base/logging.h>
#include <android/hardware/automotive/can/1.0/ICanController.h>
#include <android/hidl/manager/1.2/IServiceManager.h>
#include <hidl-utils/hidl-utils.h>

#include <iostream>
#include <string>

namespace android::hardware::automotive::can::libcanhaltools {

using ICanBus = V1_0::ICanBus;
using ICanController = V1_0::ICanController;
using IfIdDisc = ICanController::BusConfig::InterfaceId::hidl_discriminator;

hidl_vec<hidl_string> getControlServices() {
    auto manager = hidl::manager::V1_2::IServiceManager::getService();
    hidl_vec<hidl_string> services;
    manager->listManifestByInterface(ICanController::descriptor, hidl_utils::fill(&services));
    CHECK(services.size() > 0) << "No ICanController services registered (missing privileges?)"
                               << std::endl;
    return services;
}

bool isSupported(sp<ICanController> ctrl, ICanController::InterfaceType iftype) {
    hidl_vec<ICanController::InterfaceType> supported;
    if (!ctrl->getSupportedInterfaceTypes(hidl_utils::fill(&supported)).isOk()) return false;
    return supported.contains(iftype);
}

ICanController::InterfaceType getIftype(ICanController::BusConfig can_config) {
    switch (can_config.interfaceId.getDiscriminator()) {
        case IfIdDisc::socketcan:
            return ICanController::InterfaceType::SOCKETCAN;
        case IfIdDisc::slcan:
            return ICanController::InterfaceType::SLCAN;
        case IfIdDisc::virtualif:
            return ICanController::InterfaceType::VIRTUAL;
        case IfIdDisc::indexed:
            return ICanController::InterfaceType::INDEXED;
        default:
            CHECK(false) << "HAL returned unexpected interface type!";
    }
}

ICanController::Result configureIface(ICanController::BusConfig can_config) {
    auto iftype = getIftype(can_config);
    auto can_controller_list = getControlServices();
    for (auto const& service : can_controller_list) {
        auto ctrl = ICanController::getService(service);
        if (ctrl == nullptr) {
            LOG(ERROR) << "Couldn't open ICanController/" << service;
            continue;
        }

        if (!libcanhaltools::isSupported(ctrl, iftype)) continue;

        const auto up_result = ctrl->upInterface(can_config);
        if (up_result != ICanController::Result::OK) {
            LOG(ERROR) << "Failed to bring " << can_config.name << " up: " << toString(up_result)
                       << std::endl;
        }
        return up_result;
    }
    return ICanController::Result::NOT_SUPPORTED;
}

}  // namespace android::hardware::automotive::can::libcanhaltools
