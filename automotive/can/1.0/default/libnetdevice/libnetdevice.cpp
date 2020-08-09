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
#include <libnl++/MessageFactory.h>
#include <libnl++/Socket.h>

#include <linux/can.h>
#include <linux/rtnetlink.h>
#include <net/if.h>

#include <sstream>

namespace android::netdevice {

void useSocketDomain(int domain) {
    ifreqs::socketDomain = domain;
}

bool exists(std::string ifname) {
    return nametoindex(ifname) != 0;
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
    nl::MessageFactory<ifinfomsg> req(RTM_NEWLINK,
                                      NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK);
    req.add(IFLA_IFNAME, dev);

    {
        auto linkinfo = req.addNested(IFLA_LINKINFO);
        req.add(IFLA_INFO_KIND, type);
    }

    nl::Socket sock(NETLINK_ROUTE);
    return sock.send(req) && sock.receiveAck(req);
}

bool del(std::string dev) {
    nl::MessageFactory<ifinfomsg> req(RTM_DELLINK, NLM_F_REQUEST | NLM_F_ACK);
    req.add(IFLA_IFNAME, dev);

    nl::Socket sock(NETLINK_ROUTE);
    return sock.send(req) && sock.receiveAck(req);
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

std::optional<bool> isUp(std::string ifname) {
    auto ifr = ifreqs::fromName(ifname);
    if (!ifreqs::send(SIOCGIFFLAGS, ifr)) return std::nullopt;
    return ifr.ifr_flags & IFF_UP;
}

struct WaitState {
    bool present;
    bool up;

    bool satisfied(WaitCondition cnd) const {
        switch (cnd) {
            case WaitCondition::PRESENT:
                if (present) return true;
                break;
            case WaitCondition::PRESENT_AND_UP:
                if (present && up) return true;
                break;
            case WaitCondition::DOWN_OR_GONE:
                if (!present || !up) return true;
                break;
        }
        return false;
    }
};

static std::string toString(WaitCondition cnd) {
    switch (cnd) {
        case WaitCondition::PRESENT:
            return "become present";
        case WaitCondition::PRESENT_AND_UP:
            return "come up";
        case WaitCondition::DOWN_OR_GONE:
            return "go down";
    }
}

static std::string toString(const std::set<std::string>& ifnames) {
    std::stringstream ss;
    std::copy(ifnames.begin(), ifnames.end(), std::ostream_iterator<std::string>(ss, ","));
    auto str = ss.str();
    str.pop_back();
    return str;
}

void waitFor(std::set<std::string> ifnames, WaitCondition cnd, bool allOf) {
    nl::Socket sock(NETLINK_ROUTE, 0, RTMGRP_LINK);

    using StatesMap = std::map<std::string, WaitState>;
    StatesMap states = {};
    for (const auto ifname : ifnames) {
        const auto present = exists(ifname);
        const auto up = present && isUp(ifname).value_or(false);
        states[ifname] = {present, up};
    }

    const auto mapConditionChecker = [cnd](const StatesMap::iterator::value_type& it) {
        return it.second.satisfied(cnd);
    };
    const auto isFullySatisfied = [&states, allOf, mapConditionChecker]() {
        if (allOf) {
            return std::all_of(states.begin(), states.end(), mapConditionChecker);
        } else {
            return std::any_of(states.begin(), states.end(), mapConditionChecker);
        }
    };

    if (isFullySatisfied()) return;

    LOG(DEBUG) << "Waiting for " << (allOf ? "" : "any of ") << toString(ifnames) << " to "
               << toString(cnd);
    for (const auto rawMsg : sock) {
        const auto msg = nl::Message<ifinfomsg>::parse(rawMsg, {RTM_NEWLINK, RTM_DELLINK});
        if (!msg.has_value()) continue;

        const auto ifname = msg->attributes.get<std::string>(IFLA_IFNAME);
        if (ifnames.count(ifname) == 0) continue;

        const bool present = (msg->header.nlmsg_type != RTM_DELLINK);
        const bool up = present && (msg->data.ifi_flags & IFF_UP) != 0;
        states[ifname] = {present, up};

        if (isFullySatisfied()) {
            LOG(DEBUG) << "Finished waiting for " << (allOf ? "" : "some of ") << toString(ifnames)
                       << " to " << toString(cnd);
            return;
        }
    }
    LOG(FATAL) << "Can't read Netlink socket";
}

}  // namespace android::netdevice

bool operator==(const android::netdevice::hwaddr_t lhs, const unsigned char rhs[ETH_ALEN]) {
    static_assert(lhs.size() == ETH_ALEN);
    return 0 == memcmp(lhs.data(), rhs, lhs.size());
}
