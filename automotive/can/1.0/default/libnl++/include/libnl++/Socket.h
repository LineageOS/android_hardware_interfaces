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
#include <libnl++/Buffer.h>
#include <libnl++/MessageFactory.h>

#include <linux/netlink.h>

#include <optional>
#include <vector>

namespace android::nl {

/**
 * A wrapper around AF_NETLINK sockets.
 *
 * This class is not thread safe to use a single instance between multiple threads, but it's fine to
 * use multiple instances over multiple threads.
 */
class Socket {
  public:
    static constexpr size_t defaultReceiveSize = 8192;

    /**
     * Socket constructor.
     *
     * \param protocol the Netlink protocol to use.
     * \param pid port id. Default value of 0 allows the kernel to assign us a unique pid.
     *        (NOTE: this is NOT the same as process id).
     * \param groups Netlink multicast groups to listen to. This is a 32-bit bitfield, where each
     *        bit is a different group. Default value of 0 means no groups are selected.
     *        See man netlink.7.
     * for more details.
     */
    Socket(int protocol, unsigned pid = 0, uint32_t groups = 0);

    /**
     * Send Netlink message with incremented sequence number to the Kernel.
     *
     * \param msg Message to send. Its sequence number will be updated.
     * \return true, if succeeded.
     */
    template <class T, unsigned BUFSIZE>
    bool send(MessageFactory<T, BUFSIZE>& req) {
        sockaddr_nl sa = {};
        sa.nl_family = AF_NETLINK;
        sa.nl_pid = 0;  // Kernel
        return send(req, sa);
    }

    /**
     * Send Netlink message with incremented sequence number.
     *
     * \param msg Message to send. Its sequence number will be updated.
     * \param sa Destination address.
     * \return true, if succeeded.
     */
    template <class T, unsigned BUFSIZE>
    bool send(MessageFactory<T, BUFSIZE>& req, const sockaddr_nl& sa) {
        if (!req.isGood()) return false;

        const auto nlmsg = req.header();
        nlmsg->nlmsg_seq = mSeq + 1;

        // With MessageFactory<>, we trust nlmsg_len to be correct.
        return send({nlmsg, nlmsg->nlmsg_len}, sa);
    }

    /**
     * Send Netlink message.
     *
     * \param msg Message to send.
     * \param sa Destination address.
     * \return true, if succeeded.
     */
    bool send(const Buffer<nlmsghdr>& msg, const sockaddr_nl& sa);

    /**
     * Receive one or multiple Netlink messages.
     *
     * WARNING: the underlying buffer is owned by Socket class and the data is valid until the next
     * call to the read function or until deallocation of Socket instance.
     *
     * \param maxSize Maximum total size of received messages
     * \return Buffer view with message data, std::nullopt on error.
     */
    std::optional<Buffer<nlmsghdr>> receive(size_t maxSize = defaultReceiveSize);

    /**
     * Receive one or multiple Netlink messages and the sender process address.
     *
     * WARNING: the underlying buffer is owned by Socket class and the data is valid until the next
     * call to the read function or until deallocation of Socket instance.
     *
     * \param maxSize Maximum total size of received messages
     * \return A pair (for use with structured binding) containing:
     *         - buffer view with message data, std::nullopt on error;
     *         - sender process address.
     */
    std::pair<std::optional<Buffer<nlmsghdr>>, sockaddr_nl> receiveFrom(
            size_t maxSize = defaultReceiveSize);

    /**
     * Receive Netlink ACK message from Kernel.
     *
     * \return true if received ACK message, false in case of error
     */
    bool receiveAck();

    /**
     * Fetches the socket PID.
     *
     * \return PID that socket is bound to.
     */
    std::optional<unsigned> getPid();

  private:
    const int mProtocol;

    uint32_t mSeq = 0;
    base::unique_fd mFd;
    bool mFailed = false;
    std::vector<uint8_t> mReceiveBuffer;

    DISALLOW_COPY_AND_ASSIGN(Socket);
};

}  // namespace android::nl
