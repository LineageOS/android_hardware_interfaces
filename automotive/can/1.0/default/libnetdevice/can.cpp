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
#include <android-base/unique_fd.h>

#include <linux/can.h>
#include <linux/can/error.h>
#include <linux/can/netlink.h>
#include <linux/can/raw.h>

namespace android::netdevice::can {

static constexpr can_err_mask_t kErrMask = CAN_ERR_MASK;

base::unique_fd socket(const std::string& ifname) {
    struct sockaddr_can addr = {};
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
        LOG(ERROR) << "Can't receive error frames, CAN setsockpt failed: " << strerror(errno);
        return {};
    }

    if (0 != fcntl(sock.get(), F_SETFL, O_RDWR | O_NONBLOCK)) {
        LOG(ERROR) << "Couldn't put CAN socket in non-blocking mode";
        return {};
    }

    if (0 != bind(sock.get(), reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr))) {
        LOG(ERROR) << "Can't bind to CAN interface " << ifname;
        return {};
    }

    return sock;
}

bool setBitrate(std::string ifname, uint32_t bitrate) {
    struct can_bittiming bt = {};
    bt.bitrate = bitrate;

    NetlinkRequest<struct ifinfomsg> req(RTM_NEWLINK, NLM_F_REQUEST);

    const auto ifidx = nametoindex(ifname);
    if (ifidx == 0) {
        LOG(ERROR) << "Can't find interface " << ifname;
        return false;
    }
    req.data().ifi_index = ifidx;

    {
        auto linkinfo = req.nest(IFLA_LINKINFO);
        req.addattr(IFLA_INFO_KIND, "can");
        {
            auto infodata = req.nest(IFLA_INFO_DATA);
            /* For CAN FD, it would require to add IFLA_CAN_DATA_BITTIMING
             * and IFLA_CAN_CTRLMODE as well. */
            req.addattr(IFLA_CAN_BITTIMING, bt);
        }
    }

    NetlinkSocket sock(NETLINK_ROUTE);
    return sock.send(req) && sock.receiveAck();
}

}  // namespace android::netdevice::can
