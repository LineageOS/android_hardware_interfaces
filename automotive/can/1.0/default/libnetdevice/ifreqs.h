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

#include <net/if.h>

#include <string>

namespace android::netdevice::ifreqs {

/**
 * \see useSocketDomain()
 */
extern std::atomic_int socketDomain;

/**
 * Sends ioctl interface request.
 *
 * \param request Request type (such as SIOCGIFFLAGS)
 * \param ifr Request data (both input and output)
 * \return true if the call succeeded, false otherwise
 */
bool send(unsigned long request, struct ifreq& ifr);

/**
 * Initializes interface request with interface name.
 *
 * \param ifname Interface to initialize request with
 * \return Interface request with ifr_name field set to ifname
 */
struct ifreq fromName(const std::string& ifname);

}  // namespace android::netdevice::ifreqs
