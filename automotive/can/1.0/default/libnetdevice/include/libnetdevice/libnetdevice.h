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

#pragma once

#include <optional>
#include <string>

namespace android::netdevice {

/**
 * Checks, if the network interface exists.
 *
 * \param ifname Interface to check
 * \return true if it exists, false otherwise
 */
bool exists(std::string ifname);

/**
 * Checks if network interface is up.
 *
 * \param ifname Interface to check
 * \return true/false if the check succeeded, nullopt otherwise
 */
std::optional<bool> isUp(std::string ifname);

/**
 * Brings network interface up.
 *
 * \param ifname Interface to bring up
 * \return true in case of success, false otherwise
 */
bool up(std::string ifname);

/**
 * Brings network interface down.
 *
 * \param ifname Interface to bring down
 * \return true in case of success, false otherwise
 */
bool down(std::string ifname);

/**
 * Adds virtual link.
 *
 * \param dev the name of the new virtual device
 * \param type the type of the new device
 * \return true in case of success, false otherwise
 */
bool add(std::string dev, std::string type);

/**
 * Deletes virtual link.
 *
 * \param dev the name of the device to remove
 * \return true in case of success, false otherwise
 */
bool del(std::string dev);

}  // namespace android::netdevice
