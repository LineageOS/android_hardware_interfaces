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
        req.addBuffer(IFLA_INFO_KIND, type);
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

static bool hasIpv4(std::string ifname) {
    auto ifr = ifreqs::fromName(ifname);
    switch (ifreqs::trySend(SIOCGIFADDR, ifr)) {
        case 0:
            return true;
        case EADDRNOTAVAIL:
        case ENODEV:
            return false;
        default:
            PLOG(WARNING) << "Failed checking IPv4 address";
            return false;
    }
}

struct WaitState {
    bool present;
    bool up;
    bool hasIpv4Addr;

    bool satisfied(WaitCondition cnd) const {
        switch (cnd) {
            case WaitCondition::PRESENT:
                return present;
            case WaitCondition::PRESENT_AND_UP:
                return present && up;
            case WaitCondition::PRESENT_AND_IPV4:
                return present && up && hasIpv4Addr;
            case WaitCondition::DOWN_OR_GONE:
                return !present || !up;
        }
    }
};

static std::string toString(WaitCondition cnd) {
    switch (cnd) {
        case WaitCondition::PRESENT:
            return "become present";
        case WaitCondition::PRESENT_AND_UP:
            return "come up";
        case WaitCondition::PRESENT_AND_IPV4:
            return "get IPv4 address";
        case WaitCondition::DOWN_OR_GONE:
            return "go down";
    }
}

static std::string toString(Quantifier quant) {
    switch (quant) {
        case Quantifier::ALL_OF:
            return "all of";
        case Quantifier::ANY_OF:
            return "any of";
    }
}

static std::string toString(const std::set<std::string>& ifnames) {
    std::stringstream ss;
    std::copy(ifnames.begin(), ifnames.end(), std::ostream_iterator<std::string>(ss, ","));
    auto str = ss.str();
    str.pop_back();
    return str;
}

std::optional<std::string> waitFor(std::set<std::string> ifnames, WaitCondition cnd,
                                   Quantifier quant) {
    nl::Socket sock(NETLINK_ROUTE, 0, RTMGRP_LINK | RTMGRP_IPV4_IFADDR);

    using StatesMap = std::map<std::string, WaitState>;
    StatesMap states = {};
    for (const auto ifname : ifnames) {
        const auto present = exists(ifname);
        const auto up = present && isUp(ifname).value_or(false);
        const auto hasIpv4Addr = present && hasIpv4(ifname);
        states[ifname] = {present, up, hasIpv4Addr};
    }

    const auto mapConditionChecker = [cnd](const StatesMap::iterator::value_type& it) {
        return it.second.satisfied(cnd);
    };
    const auto isFullySatisfied = [&states, quant,
                                   mapConditionChecker]() -> std::optional<std::string> {
        if (quant == Quantifier::ALL_OF) {
            if (!std::all_of(states.begin(), states.end(), mapConditionChecker)) return {};
            return states.begin()->first;
        } else {  // Quantifier::ANY_OF
            const auto it = std::find_if(states.begin(), states.end(), mapConditionChecker);
            if (it == states.end()) return {};
            return it->first;
        }
    };

    if (const auto iface = isFullySatisfied()) return iface;

    LOG(DEBUG) << "Waiting for " << toString(quant) << " " << toString(ifnames) << " to "
               << toString(cnd);
    for (const auto rawMsg : sock) {
        if (const auto msg = nl::Message<ifinfomsg>::parse(rawMsg, {RTM_NEWLINK, RTM_DELLINK});
            msg.has_value()) {
            // Interface added / removed
            const auto ifname = msg->attributes.get<std::string>(IFLA_IFNAME);
            if (ifnames.count(ifname) == 0) continue;

            auto& state = states[ifname];
            state.present = (msg->header.nlmsg_type != RTM_DELLINK);
            state.up = state.present && (msg->data.ifi_flags & IFF_UP) != 0;
            if (!state.present) state.hasIpv4Addr = false;

        } else if (const auto msg =
                           nl::Message<ifaddrmsg>::parse(rawMsg, {RTM_NEWADDR, RTM_DELADDR});
                   msg.has_value()) {
            // Address added / removed
            const auto ifname = msg->attributes.get<std::string>(IFLA_IFNAME);
            if (ifnames.count(ifname) == 0) continue;

            if (msg->header.nlmsg_type == RTM_NEWADDR) {
                states[ifname].hasIpv4Addr = true;
            } else {
                // instead of tracking which one got deleted, let's just ask
                states[ifname].hasIpv4Addr = hasIpv4(ifname);
            }
        }

        if (const auto iface = isFullySatisfied()) {
            LOG(DEBUG) << "Finished waiting for " << toString(quant) << " " << toString(ifnames)
                       << " to " << toString(cnd);
            return iface;
        }
    }
    LOG(FATAL) << "Can't read Netlink socket";
    return {};
}

}  // namespace android::netdevice

bool operator==(const android::netdevice::hwaddr_t lhs, const unsigned char rhs[ETH_ALEN]) {
    static_assert(lhs.size() == ETH_ALEN);
    return 0 == memcmp(lhs.data(), rhs, lhs.size());
}
