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

#include <libnetdevice/vlan.h>

#include "common.h"

#include <android-base/logging.h>
#include <libnl++/MessageFactory.h>
#include <libnl++/Socket.h>

#include <linux/rtnetlink.h>

namespace android::netdevice::vlan {

bool add(const std::string& eth, const std::string& vlan, uint16_t id) {
    const auto ethidx = nametoindex(eth);
    if (ethidx == 0) {
        LOG(ERROR) << "Ethernet interface " << eth << " doesn't exist";
        return false;
    }

    nl::MessageFactory<ifinfomsg> req(RTM_NEWLINK,
                                      NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK);
    req.add(IFLA_IFNAME, vlan);
    req.add<uint32_t>(IFLA_LINK, ethidx);

    {
        auto linkinfo = req.addNested(IFLA_LINKINFO);
        req.add(IFLA_INFO_KIND, "vlan");

        {
            auto linkinfo = req.addNested(IFLA_INFO_DATA);
            req.add(IFLA_VLAN_ID, id);
        }
    }

    nl::Socket sock(NETLINK_ROUTE);
    return sock.send(req) && sock.receiveAck(req);
}

}  // namespace android::netdevice::vlan
