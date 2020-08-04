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

#include <libnl++/Socket.h>

#include <libnl++/printer.h>

#include <android-base/logging.h>

namespace android::nl {

/**
 * Print all outbound/inbound Netlink messages.
 */
static constexpr bool kSuperVerbose = false;

Socket::Socket(int protocol, unsigned pid, uint32_t groups) : mProtocol(protocol) {
    mFd.reset(socket(AF_NETLINK, SOCK_RAW, protocol));
    if (!mFd.ok()) {
        PLOG(ERROR) << "Can't open Netlink socket";
        mFailed = true;
        return;
    }

    sockaddr_nl sa = {};
    sa.nl_family = AF_NETLINK;
    sa.nl_pid = pid;
    sa.nl_groups = groups;

    if (bind(mFd.get(), reinterpret_cast<sockaddr*>(&sa), sizeof(sa)) < 0) {
        PLOG(ERROR) << "Can't bind Netlink socket";
        mFd.reset();
        mFailed = true;
    }
}

bool Socket::send(const Buffer<nlmsghdr>& msg, const sockaddr_nl& sa) {
    if constexpr (kSuperVerbose) {
        LOG(VERBOSE) << (mFailed ? "(not) " : "") << "sending Netlink message ("  //
                     << msg->nlmsg_pid << " -> " << sa.nl_pid << "): " << toString(msg, mProtocol);
    }
    if (mFailed) return false;

    mSeq = msg->nlmsg_seq;
    const auto rawMsg = msg.getRaw();
    const auto bytesSent = sendto(mFd.get(), rawMsg.ptr(), rawMsg.len(), 0,
                                  reinterpret_cast<const sockaddr*>(&sa), sizeof(sa));
    if (bytesSent < 0) {
        PLOG(ERROR) << "Can't send Netlink message";
        return false;
    } else if (size_t(bytesSent) != rawMsg.len()) {
        LOG(ERROR) << "Can't send Netlink message: truncated message";
        return false;
    }
    return true;
}

std::optional<Buffer<nlmsghdr>> Socket::receive(size_t maxSize) {
    return receiveFrom(maxSize).first;
}

std::pair<std::optional<Buffer<nlmsghdr>>, sockaddr_nl> Socket::receiveFrom(size_t maxSize) {
    if (mFailed) return {std::nullopt, {}};

    if (maxSize == 0) {
        LOG(ERROR) << "Maximum receive size should not be zero";
        return {std::nullopt, {}};
    }
    if (mReceiveBuffer.size() < maxSize) mReceiveBuffer.resize(maxSize);

    sockaddr_nl sa = {};
    socklen_t saLen = sizeof(sa);
    const auto bytesReceived = recvfrom(mFd.get(), mReceiveBuffer.data(), maxSize, MSG_TRUNC,
                                        reinterpret_cast<sockaddr*>(&sa), &saLen);

    if (bytesReceived <= 0) {
        PLOG(ERROR) << "Failed to receive Netlink message";
        return {std::nullopt, {}};
    } else if (size_t(bytesReceived) > maxSize) {
        PLOG(ERROR) << "Received data larger than maximum receive size: "  //
                    << bytesReceived << " > " << maxSize;
        return {std::nullopt, {}};
    }

    Buffer<nlmsghdr> msg(reinterpret_cast<nlmsghdr*>(mReceiveBuffer.data()), bytesReceived);
    if constexpr (kSuperVerbose) {
        LOG(VERBOSE) << "received (" << sa.nl_pid << " -> " << msg->nlmsg_pid << "):"  //
                     << toString(msg, mProtocol);
    }
    return {msg, sa};
}

/* TODO(161389935): Migrate receiveAck to use nlmsg<> internally. Possibly reuse
 * Socket::receive(). */
bool Socket::receiveAck() {
    if (mFailed) return false;

    char buf[8192];

    sockaddr_nl sa;
    iovec iov = {buf, sizeof(buf)};

    msghdr msg = {};
    msg.msg_name = &sa;
    msg.msg_namelen = sizeof(sa);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    const ssize_t status = recvmsg(mFd.get(), &msg, 0);
    if (status < 0) {
        PLOG(ERROR) << "Failed to receive Netlink message";
        return false;
    }
    size_t remainingLen = status;

    if (msg.msg_flags & MSG_TRUNC) {
        LOG(ERROR) << "Failed to receive Netlink message: truncated";
        return false;
    }

    for (auto nlmsg = reinterpret_cast<nlmsghdr*>(buf); NLMSG_OK(nlmsg, remainingLen);
         nlmsg = NLMSG_NEXT(nlmsg, remainingLen)) {
        if constexpr (kSuperVerbose) {
            LOG(VERBOSE) << "received Netlink response: "
                         << toString({nlmsg, nlmsg->nlmsg_len}, mProtocol);
        }

        // We're looking for error/ack message only, ignoring others.
        if (nlmsg->nlmsg_type != NLMSG_ERROR) {
            LOG(WARNING) << "Received unexpected Netlink message (ignored): " << nlmsg->nlmsg_type;
            continue;
        }

        // Found error/ack message, return status.
        const auto nlerr = reinterpret_cast<nlmsgerr*>(NLMSG_DATA(nlmsg));
        if (nlerr->error != 0) {
            LOG(ERROR) << "Received Netlink error message: " << strerror(-nlerr->error);
            return false;
        }
        return true;
    }
    // Couldn't find any error/ack messages.
    return false;
}

std::optional<unsigned> Socket::getPid() {
    sockaddr_nl sa = {};
    socklen_t sasize = sizeof(sa);
    if (getsockname(mFd.get(), reinterpret_cast<sockaddr*>(&sa), &sasize) < 0) {
        PLOG(ERROR) << "Failed to get PID of Netlink socket";
        return std::nullopt;
    }
    return sa.nl_pid;
}

}  // namespace android::nl
