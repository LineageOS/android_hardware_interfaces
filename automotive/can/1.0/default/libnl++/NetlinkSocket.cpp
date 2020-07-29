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

#include <libnl++/NetlinkSocket.h>

#include <libnl++/printer.h>

#include <android-base/logging.h>

namespace android::nl {

/**
 * Print all outbound/inbound Netlink messages.
 */
static constexpr bool kSuperVerbose = false;

NetlinkSocket::NetlinkSocket(int protocol, unsigned int pid, uint32_t groups)
    : mProtocol(protocol) {
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

bool NetlinkSocket::send(nlmsghdr* nlmsg, size_t totalLen) {
    if constexpr (kSuperVerbose) {
        nlmsg->nlmsg_seq = mSeq;
        LOG(VERBOSE) << (mFailed ? "(not) " : "")
                     << "sending Netlink message: " << toString({nlmsg, totalLen}, mProtocol);
    }

    if (mFailed) return false;

    nlmsg->nlmsg_pid = 0;  // kernel
    nlmsg->nlmsg_seq = mSeq++;
    nlmsg->nlmsg_flags |= NLM_F_ACK;

    iovec iov = {nlmsg, nlmsg->nlmsg_len};

    sockaddr_nl sa = {};
    sa.nl_family = AF_NETLINK;

    msghdr msg = {};
    msg.msg_name = &sa;
    msg.msg_namelen = sizeof(sa);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    if (sendmsg(mFd.get(), &msg, 0) < 0) {
        PLOG(ERROR) << "Can't send Netlink message";
        return false;
    }
    return true;
}

bool NetlinkSocket::send(const nlbuf<nlmsghdr>& msg, const sockaddr_nl& sa) {
    if constexpr (kSuperVerbose) {
        LOG(VERBOSE) << (mFailed ? "(not) " : "")
                     << "sending Netlink message: " << toString(msg, mProtocol);
    }

    if (mFailed) return false;
    const auto rawMsg = msg.getRaw();
    const auto bytesSent = sendto(mFd.get(), rawMsg.ptr(), rawMsg.len(), 0,
                                  reinterpret_cast<const sockaddr*>(&sa), sizeof(sa));
    if (bytesSent < 0) {
        PLOG(ERROR) << "Can't send Netlink message";
        return false;
    }
    return true;
}

std::optional<nlbuf<nlmsghdr>> NetlinkSocket::receive(void* buf, size_t bufLen) {
    sockaddr_nl sa = {};
    return receive(buf, bufLen, sa);
}

std::optional<nlbuf<nlmsghdr>> NetlinkSocket::receive(void* buf, size_t bufLen, sockaddr_nl& sa) {
    if (mFailed) return std::nullopt;

    socklen_t saLen = sizeof(sa);
    if (bufLen == 0) {
        LOG(ERROR) << "Receive buffer has zero size!";
        return std::nullopt;
    }
    const auto bytesReceived =
            recvfrom(mFd.get(), buf, bufLen, MSG_TRUNC, reinterpret_cast<sockaddr*>(&sa), &saLen);
    if (bytesReceived <= 0) {
        PLOG(ERROR) << "Failed to receive Netlink message";
        return std::nullopt;
    } else if (unsigned(bytesReceived) > bufLen) {
        PLOG(ERROR) << "Received data larger than the receive buffer! " << bytesReceived << " > "
                    << bufLen;
        return std::nullopt;
    }

    nlbuf<nlmsghdr> msg(reinterpret_cast<nlmsghdr*>(buf), bytesReceived);
    if constexpr (kSuperVerbose) {
        LOG(VERBOSE) << "received " << toString(msg, mProtocol);
    }
    return msg;
}

/* TODO(161389935): Migrate receiveAck to use nlmsg<> internally. Possibly reuse
 * NetlinkSocket::receive(). */
bool NetlinkSocket::receiveAck() {
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

std::optional<unsigned int> NetlinkSocket::getSocketPid() {
    sockaddr_nl sa = {};
    socklen_t sasize = sizeof(sa);
    if (getsockname(mFd.get(), reinterpret_cast<sockaddr*>(&sa), &sasize) < 0) {
        PLOG(ERROR) << "Failed to getsockname() for netlink_fd!";
        return std::nullopt;
    }
    return sa.nl_pid;
}

}  // namespace android::nl
