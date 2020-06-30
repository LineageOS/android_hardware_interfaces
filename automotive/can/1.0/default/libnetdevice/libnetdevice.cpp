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

#include <libnetdevice/libnetdevice.h>

#include "common.h"
#include "ifreqs.h"

#include <android-base/logging.h>
#include <libnetdevice/NetlinkRequest.h>
#include <libnetdevice/NetlinkSocket.h>

#include <linux/can.h>
#include <net/if.h>

namespace android::netdevice {

void useCanSockets(bool yes) {
    socketparams::current = yes ? socketparams::can : socketparams::general;
}

bool exists(std::string ifname) {
    return nametoindex(ifname) != 0;
}

std::optional<bool> isUp(std::string ifname) {
    auto ifr = ifreqs::fromName(ifname);
    if (!ifreqs::send(SIOCGIFFLAGS, ifr)) return std::nullopt;
    return ifr.ifr_flags & IFF_UP;
}

bool existsAndIsUp(const std::string& ifname) {
    return exists(ifname) && isUp(ifname).value_or(false);
}

bool up(std::string ifname) {
    auto ifr = ifreqs::fromName(ifname);
    if (!ifreqs::send(SIOCGIFFLAGS, ifr)) return false;
    ifr.ifr_flags |= IFF_UP;
    return ifreqs::send(SIOCSIFFLAGS, ifr);
}

bool down(std::string ifname) {
    auto ifr = ifreqs::fromName(ifname);
    if (!ifreqs::send(SIOCGIFFLAGS, ifr)) return false;
    ifr.ifr_flags &= ~IFF_UP;
    return ifreqs::send(SIOCSIFFLAGS, ifr);
}

bool add(std::string dev, std::string type) {
    NetlinkRequest<struct ifinfomsg> req(RTM_NEWLINK, NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL);
    req.addattr(IFLA_IFNAME, dev);

    {
        auto linkinfo = req.nest(IFLA_LINKINFO);
        req.addattr(IFLA_INFO_KIND, type);
    }

    NetlinkSocket sock(NETLINK_ROUTE);
    return sock.send(req) && sock.receiveAck();
}

bool del(std::string dev) {
    NetlinkRequest<struct ifinfomsg> req(RTM_DELLINK, NLM_F_REQUEST);
    req.addattr(IFLA_IFNAME, dev);

    NetlinkSocket sock(NETLINK_ROUTE);
    return sock.send(req) && sock.receiveAck();
}

std::optional<hwaddr_t> getHwAddr(const std::string& ifname) {
    auto ifr = ifreqs::fromName(ifname);
    if (!ifreqs::send(SIOCGIFHWADDR, ifr)) return std::nullopt;

    hwaddr_t hwaddr;
    memcpy(hwaddr.data(), ifr.ifr_hwaddr.sa_data, hwaddr.size());
    return hwaddr;
}

bool setHwAddr(const std::string& ifname, hwaddr_t hwaddr) {
    auto ifr = ifreqs::fromName(ifname);

    // fetch sa_family
    if (!ifreqs::send(SIOCGIFHWADDR, ifr)) return false;

    memcpy(ifr.ifr_hwaddr.sa_data, hwaddr.data(), hwaddr.size());
    return ifreqs::send(SIOCSIFHWADDR, ifr);
}

}  // namespace android::netdevice

bool operator==(const android::netdevice::hwaddr_t lhs, const unsigned char rhs[ETH_ALEN]) {
    static_assert(lhs.size() == ETH_ALEN);
    return 0 == memcmp(lhs.data(), rhs, lhs.size());
}
