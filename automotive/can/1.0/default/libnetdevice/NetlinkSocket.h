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

#include "NetlinkRequest.h"

#include <android-base/macros.h>
#include <android-base/unique_fd.h>

#include <linux/netlink.h>

namespace android::netdevice {

/**
 * A wrapper around AF_NETLINK sockets.
 *
 * This class is not thread safe to use a single instance between multiple threads, but it's fine to
 * use multiple instances over multiple threads.
 */
struct NetlinkSocket {
    NetlinkSocket(int protocol);

    /**
     * Send Netlink message to Kernel.
     *
     * \param msg Message to send, nlmsg_seq will be set to next sequence number
     * \return true, if succeeded
     */
    template <class T, unsigned int BUFSIZE>
    bool send(NetlinkRequest<T, BUFSIZE>& req) {
        if (!req.isGood()) return false;
        return send(req.header());
    }

    /**
     * Receive Netlink ACK message from Kernel.
     *
     * \return true if received ACK message, false in case of error
     */
    bool receiveAck();

  private:
    uint32_t mSeq = 0;
    base::unique_fd mFd;
    bool mFailed = false;

    bool send(struct nlmsghdr* msg);

    DISALLOW_COPY_AND_ASSIGN(NetlinkSocket);
};

}  // namespace android::netdevice
