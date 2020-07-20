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

#include "ifreqs.h"

#include "common.h"

#include <android-base/logging.h>
#include <android-base/unique_fd.h>

#include <map>

namespace android::netdevice::ifreqs {

static constexpr int defaultSocketDomain = AF_INET;
std::atomic_int socketDomain = defaultSocketDomain;

struct SocketParams {
    int domain;
    int type;
    int protocol;
};

static const std::map<int, SocketParams> socketParams = {
        {AF_INET, {AF_INET, SOCK_DGRAM, 0}},
        {AF_CAN, {AF_CAN, SOCK_RAW, CAN_RAW}},
};

static SocketParams getSocketParams(int domain) {
    if (socketParams.count(domain)) return socketParams.find(domain)->second;

    auto params = socketParams.find(defaultSocketDomain)->second;
    params.domain = domain;
    return params;
}

bool send(unsigned long request, struct ifreq& ifr) {
    const auto sp = getSocketParams(socketDomain);
    base::unique_fd sock(socket(sp.domain, sp.type, sp.protocol));
    if (!sock.ok()) {
        LOG(ERROR) << "Can't create socket";
        return false;
    }

    if (ioctl(sock.get(), request, &ifr) < 0) {
        PLOG(ERROR) << "ioctl(" << std::hex << request << std::dec << ") failed";
        return false;
    }

    return true;
}

struct ifreq fromName(const std::string& ifname) {
    struct ifreq ifr = {};
    strlcpy(ifr.ifr_name, ifname.c_str(), IF_NAMESIZE);
    return ifr;
}

}  // namespace android::netdevice::ifreqs
