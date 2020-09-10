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
#include <libnl++/Message.h>
#include <libnl++/MessageFactory.h>

#include <linux/netlink.h>
#include <poll.h>

#include <optional>
#include <set>
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
    template <typename T, unsigned BUFSIZE>
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
    template <typename T, unsigned BUFSIZE>
    bool send(MessageFactory<T, BUFSIZE>& req, const sockaddr_nl& sa) {
        req.header.nlmsg_seq = mSeq + 1;

        const auto msg = req.build();
        if (!msg.has_value()) return false;

        return send(*msg, sa);
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
     * \param maxSize Maximum total size of received messages.
     * \return A pair (for use with structured binding) containing:
     *         - buffer view with message data, std::nullopt on error;
     *         - sender process address.
     */
    std::pair<std::optional<Buffer<nlmsghdr>>, sockaddr_nl> receiveFrom(
            size_t maxSize = defaultReceiveSize);

    /**
     * Receive matching Netlink message of a given payload type.
     *
     * This method should be used if the caller expects exactly one incoming message of exactly
     * given type (such as ACK). If there is a use case to handle multiple types of messages,
     * please use receive(size_t) directly and iterate through potential multipart messages.
     *
     * If this method is used in such an environment, it will only return the first matching message
     * from multipart packet and will issue warnings on messages that do not match.
     *
     * \param msgtypes Expected message types (such as NLMSG_ERROR).
     * \param maxSize Maximum total size of received messages.
     * \return Parsed message or std::nullopt in case of error.
     */
    template <typename T>
    std::optional<Message<T>> receive(const std::set<nlmsgtype_t>& msgtypes,
                                      size_t maxSize = defaultReceiveSize) {
        const auto msg = receive(msgtypes, maxSize);
        if (!msg.has_value()) return std::nullopt;

        const auto parsed = Message<T>::parse(*msg);
        if (!parsed.has_value()) {
            LOG(WARNING) << "Received matching Netlink message, but couldn't parse it";
            return std::nullopt;
        }

        return parsed;
    }

    /**
     * Receive Netlink ACK message.
     *
     * \param req Message to match sequence number against.
     * \return true if received ACK message, false in case of error.
     */
    template <typename T, unsigned BUFSIZE>
    bool receiveAck(MessageFactory<T, BUFSIZE>& req) {
        return receiveAck(req.header.nlmsg_seq);
    }

    /**
     * Receive Netlink ACK message.
     *
     * \param seq Sequence number of message to ACK.
     * \return true if received ACK message, false in case of error.
     */
    bool receiveAck(uint32_t seq);

    /**
     * Fetches the socket PID.
     *
     * \return PID that socket is bound to or std::nullopt.
     */
    std::optional<unsigned> getPid();

    /**
     * Creates a pollfd object for the socket.
     *
     * \param events Value for pollfd.events.
     * \return A populated pollfd object.
     */
    pollfd preparePoll(short events = 0);

    /**
     * Live iterator continuously receiving messages from Netlink socket.
     *
     * Iteration ends when socket fails to receive a buffer.
     *
     * Example:
     * ```
     *     nl::Socket sock(NETLINK_ROUTE, 0, RTMGRP_LINK);
     *     for (const auto rawMsg : sock) {
     *         const auto msg = nl::Message<ifinfomsg>::parse(rawMsg, {RTM_NEWLINK, RTM_DELLINK});
     *         if (!msg.has_value()) continue;
     *
     *         LOG(INFO) << msg->attributes.get<std::string>(IFLA_IFNAME)
     *                   << " is " << ((msg->data.ifi_flags & IFF_UP) ? "up" : "down");
     *     }
     *     LOG(FATAL) << "Failed to read from Netlink socket";
     * ```
     */
    class receive_iterator {
      public:
        receive_iterator(Socket& socket, bool end);

        receive_iterator operator++();
        bool operator==(const receive_iterator& other) const;
        const Buffer<nlmsghdr>& operator*() const;

      private:
        Socket& mSocket;
        bool mIsEnd;
        Buffer<nlmsghdr>::iterator mCurrent;

        void receive();
    };
    receive_iterator begin();
    receive_iterator end();

  private:
    const int mProtocol;
    base::unique_fd mFd;
    std::vector<uint8_t> mReceiveBuffer;

    bool mFailed = false;
    uint32_t mSeq = 0;

    bool increaseReceiveBuffer(size_t maxSize);
    std::optional<Buffer<nlmsghdr>> receive(const std::set<nlmsgtype_t>& msgtypes, size_t maxSize);

    DISALLOW_COPY_AND_ASSIGN(Socket);
};

}  // namespace android::nl
