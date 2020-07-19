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

#include <android-base/macros.h>
#include <android-base/unique_fd.h>
#include <libnetdevice/NetlinkRequest.h>
#include <libnetdevice/nlbuf.h>

#include <linux/netlink.h>

#include <optional>

namespace android::netdevice {

/**
 * A wrapper around AF_NETLINK sockets.
 *
 * This class is not thread safe to use a single instance between multiple threads, but it's fine to
 * use multiple instances over multiple threads.
 */
struct NetlinkSocket {
    /**
     * NetlinkSocket constructor.
     *
     * \param protocol the Netlink protocol to use.
     * \param pid port id. Default value of 0 allows the kernel to assign us a unique pid. (NOTE:
     * this is NOT the same as process id!)
     * \param groups Netlink multicast groups to listen to. This is a 32-bit bitfield, where each
     * bit is a different group. Default value of 0 means no groups are selected. See man netlink.7
     * for more details.
     */
    NetlinkSocket(int protocol, unsigned int pid = 0, uint32_t groups = 0);

    /**
     * Send Netlink message to Kernel. The sequence number will be automatically incremented, and
     * the NLM_F_ACK (request ACK) flag will be set.
     *
     * \param msg Message to send.
     * \return true, if succeeded
     */
    template <class T, unsigned int BUFSIZE>
    bool send(NetlinkRequest<T, BUFSIZE>& req) {
        if (!req.isGood()) return false;
        return send(req.header(), req.totalLength);
    }

    /**
     * Send Netlink message. The message will be sent as is, without any modification.
     *
     * \param msg Message to send.
     * \param sa Destination address.
     * \return true, if succeeded
     */
    bool send(const nlbuf<nlmsghdr>& msg, const sockaddr_nl& sa);

    /**
     * Receive Netlink data.
     *
     * \param buf buffer to hold message data.
     * \param bufLen length of buf.
     * \return nlbuf with message data, std::nullopt on error.
     */
    std::optional<nlbuf<nlmsghdr>> receive(void* buf, size_t bufLen);

    /**
     * Receive Netlink data with address info.
     *
     * \param buf buffer to hold message data.
     * \param bufLen length of buf.
     * \param sa Blank struct that recvfrom will populate with address info.
     * \return nlbuf with message data, std::nullopt on error.
     */
    std::optional<nlbuf<nlmsghdr>> receive(void* buf, size_t bufLen, sockaddr_nl& sa);

    /**
     * Receive Netlink ACK message from Kernel.
     *
     * \return true if received ACK message, false in case of error
     */
    bool receiveAck();

    /**
     * Gets the PID assigned to mFd.
     *
     * \return pid that mSocket is bound to.
     */
    std::optional<unsigned int> getSocketPid();

  private:
    const int mProtocol;

    uint32_t mSeq = 0;
    base::unique_fd mFd;
    bool mFailed = false;

    bool send(nlmsghdr* msg, size_t totalLen);

    DISALLOW_COPY_AND_ASSIGN(NetlinkSocket);
};

}  // namespace android::netdevice
