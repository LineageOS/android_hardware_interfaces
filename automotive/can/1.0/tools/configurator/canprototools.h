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

#include "canbus_config.pb.h"

#include <android/hardware/automotive/can/1.0/ICanController.h>

namespace android::hardware::automotive::can::config {

/**
 * This reads the protobuf config file into a protobuf object. Both text based protobuf files as
 * well as binary format protobuf files are supported.
 *
 * \param filepath string containing the name of the config file to read.
 * \return a CanBusConfig protobuf object constructed from the config file.
 */
std::optional<CanBusConfig> parseConfigFile(const std::string& filepath);

/**
 * Converts protobuf format single-bus config object to a HAL bus config object.
 *
 * \param pb_bus is the protobuf object representing a the configuration of one CAN bus.
 * \return a converted HAL bus config object.
 */
std::optional<V1_0::ICanController::BusConfig> fromPbBus(const Bus& pb_bus);

/**
 * Get the CAN HAL interface type specified by a given protobuf config object.
 *
 * \param pb_bus is the protobuf object representing a the configuration of one CAN bus.
 * \return the CAN HAL interface type.
 */
std::optional<V1_0::ICanController::InterfaceType> getHalIftype(const Bus& pb_bus);

}  // namespace android::hardware::automotive::can::config
