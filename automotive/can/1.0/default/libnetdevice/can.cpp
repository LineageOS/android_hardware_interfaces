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

#include <libnetdevice/can.h>

#include "common.h"

#include <android-base/logging.h>
#include <android-base/unique_fd.h>
#include <libnl++/MessageFactory.h>
#include <libnl++/Socket.h>

#include <linux/can.h>
#include <linux/can/error.h>
#include <linux/can/netlink.h>
#include <linux/can/raw.h>
#include <linux/rtnetlink.h>

namespace android::netdevice::can {

static constexpr can_err_mask_t kErrMask = CAN_ERR_MASK;

base::unique_fd socket(const std::string& ifname) {
    sockaddr_can addr = {};
    addr.can_family = AF_CAN;
    addr.can_ifindex = nametoindex(ifname);
    if (addr.can_ifindex == 0) {
        LOG(ERROR) << "Interface " << ifname << " doesn't exists";
        return {};
    }

    base::unique_fd sock(::socket(PF_CAN, SOCK_RAW, CAN_RAW));
    if (!sock.ok()) {
        LOG(ERROR) << "Failed to create CAN socket";
        return {};
    }

    if (setsockopt(sock.get(), SOL_CAN_RAW, CAN_RAW_ERR_FILTER, &kErrMask, sizeof(kErrMask)) < 0) {
        PLOG(ERROR) << "Can't receive error frames, CAN setsockpt failed";
        return {};
    }

    if (0 != fcntl(sock.get(), F_SETFL, O_RDWR | O_NONBLOCK)) {
        LOG(ERROR) << "Couldn't put CAN socket in non-blocking mode";
        return {};
    }

    if (0 != bind(sock.get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr))) {
        LOG(ERROR) << "Can't bind to CAN interface " << ifname;
        return {};
    }

    return sock;
}

bool setBitrate(std::string ifname, uint32_t bitrate) {
    can_bittiming bt = {};
    bt.bitrate = bitrate;

    nl::MessageFactory<ifinfomsg> req(RTM_NEWLINK, NLM_F_REQUEST | NLM_F_ACK);

    req->ifi_index = nametoindex(ifname);
    if (req->ifi_index == 0) {
        LOG(ERROR) << "Can't find interface " << ifname;
        return false;
    }

    {
        auto linkinfo = req.addNested(IFLA_LINKINFO);
        req.add(IFLA_INFO_KIND, "can");
        {
            auto infodata = req.addNested(IFLA_INFO_DATA);
            /* For CAN FD, it would require to add IFLA_CAN_DATA_BITTIMING
             * and IFLA_CAN_CTRLMODE as well. */
            req.add(IFLA_CAN_BITTIMING, bt);
        }
    }

    nl::Socket sock(NETLINK_ROUTE);
    return sock.send(req) && sock.receiveAck(req);
}

}  // namespace android::netdevice::can
