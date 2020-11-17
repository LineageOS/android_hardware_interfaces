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

#include <OffloadControlTestUtils.h>
#include <android-base/unique_fd.h>

using android::base::unique_fd;

inline const sockaddr* asSockaddr(const sockaddr_nl* nladdr) {
    return reinterpret_cast<const sockaddr*>(nladdr);
}

int conntrackSocket(unsigned groups) {
    unique_fd s(socket(AF_NETLINK, SOCK_DGRAM, NETLINK_NETFILTER));
    if (s.get() < 0) {
        return -errno;
    }

    const struct sockaddr_nl bind_addr = {
            .nl_family = AF_NETLINK,
            .nl_pad = 0,
            .nl_pid = 0,
            .nl_groups = groups,
    };
    if (::bind(s.get(), asSockaddr(&bind_addr), sizeof(bind_addr)) < 0) {
        return -errno;
    }

    const struct sockaddr_nl kernel_addr = {
            .nl_family = AF_NETLINK,
            .nl_pad = 0,
            .nl_pid = 0,
            .nl_groups = groups,
    };
    if (connect(s.get(), asSockaddr(&kernel_addr), sizeof(kernel_addr)) != 0) {
        return -errno;
    }

    return s.release();
}