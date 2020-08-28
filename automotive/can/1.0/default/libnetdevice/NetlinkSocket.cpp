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

#include "NetlinkSocket.h"

#include <android-base/logging.h>

namespace android::netdevice {

NetlinkSocket::NetlinkSocket(int protocol) {
    mFd.reset(socket(AF_NETLINK, SOCK_RAW, protocol));
    if (!mFd.ok()) {
        PLOG(ERROR) << "Can't open Netlink socket";
        mFailed = true;
        return;
    }

    struct sockaddr_nl sa = {};
    sa.nl_family = AF_NETLINK;

    if (bind(mFd.get(), reinterpret_cast<struct sockaddr*>(&sa), sizeof(sa)) < 0) {
        PLOG(ERROR) << "Can't bind Netlink socket";
        mFd.reset();
        mFailed = true;
    }
}

bool NetlinkSocket::send(struct nlmsghdr* nlmsg) {
    if (mFailed) return false;

    nlmsg->nlmsg_pid = 0;  // kernel
    nlmsg->nlmsg_seq = mSeq++;
    nlmsg->nlmsg_flags |= NLM_F_ACK;

    struct iovec iov = {nlmsg, nlmsg->nlmsg_len};

    struct sockaddr_nl sa = {};
    sa.nl_family = AF_NETLINK;

    struct msghdr msg = {};
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

bool NetlinkSocket::receiveAck() {
    if (mFailed) return false;

    char buf[8192];

    struct sockaddr_nl sa;
    struct iovec iov = {buf, sizeof(buf)};

    struct msghdr msg = {};
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

    for (auto nlmsg = reinterpret_cast<struct nlmsghdr*>(buf); NLMSG_OK(nlmsg, remainingLen);
         nlmsg = NLMSG_NEXT(nlmsg, remainingLen)) {
        // We're looking for error/ack message only, ignoring others.
        if (nlmsg->nlmsg_type != NLMSG_ERROR) {
            LOG(WARNING) << "Received unexpected Netlink message (ignored): " << nlmsg->nlmsg_type;
            continue;
        }

        // Found error/ack message, return status.
        auto nlerr = reinterpret_cast<struct nlmsgerr*>(NLMSG_DATA(nlmsg));
        if (nlerr->error != 0) {
            LOG(ERROR) << "Received Netlink error message: " << nlerr->error;
            return false;
        }
        return true;
    }
    // Couldn't find any error/ack messages.
    return false;
}

}  // namespace android::netdevice
