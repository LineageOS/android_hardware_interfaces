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
#include <string>

namespace android::netdevice::ethtool {

/**
 * Fetch a single value with ethtool_value.
 *
 * \see linux/ethtool.h
 * \param ifname Interface to fetch data for
 * \param command Fetch command (ETHTOOL_G*)
 * \return value, or nullopt if fetch failed
 */
std::optional<uint32_t> getValue(const std::string& ifname, uint32_t command);

/**
 * Set a single value with ethtool_value.
 *
 * \see linux/ethtool.h
 * \param ifname Interface to set data for
 * \param command Set command (ETHTOOL_S*)
 * \param value New value
 * \return true if succeeded, false otherwise
 */
bool setValue(const std::string& ifname, uint32_t command, uint32_t value);

}  // namespace android::netdevice::ethtool
