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

#include <linux/if_ether.h>

#include <array>
#include <optional>
#include <set>
#include <string>

namespace android::netdevice {

typedef std::array<uint8_t, ETH_ALEN> hwaddr_t;

/**
 * Configures libnetdevice to use other socket domain than AF_INET,
 * what requires less permissive SEPolicy rules for a given process.
 *
 * In such case, the process would only be able to control interfaces of a given kind.

 * \param domain Socket domain to use (e.g. AF_CAN), see socket(2) for details
 */
void useSocketDomain(int domain);

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
 * Interface condition to wait for.
 */
enum class WaitCondition {
    /**
     * Interface is present (but not necessarily up).
     */
    PRESENT,

    /**
     * Interface is up.
     */
    PRESENT_AND_UP,

    /**
     * Interface is down or not present (disconnected) at all.
     */
    DOWN_OR_GONE,
};

/**
 * Listens for interface changes until anticipated condition takes place.
 *
 * \param ifnames List of interfaces to watch for.
 * \param cnd Awaited condition.
 * \param allOf true if all interfaces need to satisfy the condition, false if only one satistying
 *        interface should stop the wait.
 */
void waitFor(std::set<std::string> ifnames, WaitCondition cnd, bool allOf = true);

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

/**
 * Fetches interface's hardware address.
 *
 * \param ifname Interface name
 * \return Hardware address (MAC address) or nullopt if the lookup failed
 */
std::optional<hwaddr_t> getHwAddr(const std::string& ifname);

/**
 * Changes interface's hardware address.
 *
 * \param ifname Interface name
 * \param hwaddr New hardware address to set
 */
bool setHwAddr(const std::string& ifname, hwaddr_t hwaddr);

}  // namespace android::netdevice

bool operator==(const android::netdevice::hwaddr_t lhs, const unsigned char rhs[ETH_ALEN]);
