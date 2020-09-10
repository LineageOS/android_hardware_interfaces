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

#include <android/hardware/automotive/can/1.0/ICanBus.h>
#include <android/hardware/automotive/can/1.0/ICanController.h>

namespace android::hardware::automotive::can::libcanhaltools {

/**
 * Fetch the list of registered can controller services.
 *
 * \return list of service names identifying the registered can controllers.
 */
hidl_vec<hidl_string> getControlServices();

/**
 * Determine if an can controller supports a specific interface type.
 *
 * \param ctrl a pointer to a can controller instance to check for interface support.
 * \param iftype the interface type we wish to check if ctrl supports.
 * \return true if iftype is supported by ctrl, false if not supported.
 */
bool isSupported(sp<V1_0::ICanController> ctrl, V1_0::ICanController::InterfaceType iftype);

/**
 * Configures a CAN interface through the CAN HAL and brings it up.
 *
 * \param can_config this holds the parameters for configuring a CAN bus.
 * \return status passed back from the CAN HAL, should be OK on success.
 */
V1_0::ICanController::Result configureIface(V1_0::ICanController::BusConfig can_config);

}  // namespace android::hardware::automotive::can::libcanhaltools
