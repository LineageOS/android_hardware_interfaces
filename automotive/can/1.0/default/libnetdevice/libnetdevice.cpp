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

#include "NetlinkRequest.h"
#include "NetlinkSocket.h"
#include "common.h"

#include <android-base/logging.h>

#include <linux/can.h>
#include <net/if.h>

namespace android::netdevice {

bool exists(std::string ifname) {
    return nametoindex(ifname) != 0;
}

static bool sendIfreq(unsigned long request, struct ifreq& ifr) {
    /* For general interfaces it would be socket(AF_INET, SOCK_DGRAM, 0),
     * but SEPolicy forces us to limit our flexibility here. */
    base::unique_fd sock(socket(PF_CAN, SOCK_RAW, CAN_RAW));
    if (!sock.ok()) {
        LOG(ERROR) << "Can't create socket";
        return false;
    }

    if (ioctl(sock.get(), request, &ifr) < 0) {
        LOG(ERROR) << "ioctl(" << std::hex << request << std::dec << ") failed: " << errno;
        return false;
    }

    return true;
}

static struct ifreq ifreqFromName(const std::string& ifname) {
    struct ifreq ifr = {};
    strlcpy(ifr.ifr_name, ifname.c_str(), IF_NAMESIZE);
    return ifr;
}

std::optional<bool> isUp(std::string ifname) {
    struct ifreq ifr = ifreqFromName(ifname);
    if (!sendIfreq(SIOCGIFFLAGS, ifr)) return std::nullopt;
    return ifr.ifr_flags & IFF_UP;
}

bool up(std::string ifname) {
    struct ifreq ifr = ifreqFromName(ifname);
    if (!sendIfreq(SIOCGIFFLAGS, ifr)) return false;
    ifr.ifr_flags |= IFF_UP;
    return sendIfreq(SIOCSIFFLAGS, ifr);
}

bool down(std::string ifname) {
    struct ifreq ifr = ifreqFromName(ifname);
    if (!sendIfreq(SIOCGIFFLAGS, ifr)) return false;
    ifr.ifr_flags &= ~IFF_UP;
    return sendIfreq(SIOCSIFFLAGS, ifr);
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

}  // namespace android::netdevice
