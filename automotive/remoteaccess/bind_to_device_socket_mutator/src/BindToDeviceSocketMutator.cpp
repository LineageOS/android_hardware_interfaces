/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include "BindToDeviceSocketMutator.h"

#include <android-base/logging.h>
#include <errno.h>
#include <src/core/lib/iomgr/socket_mutator.h>
#include <sys/socket.h>

#include <memory>

namespace android::hardware::automotive::remoteaccess {
namespace {

struct BindToDeviceMutator : grpc_socket_mutator {
    std::string ifname;
};

bool MutateFd(int fd, grpc_socket_mutator* mutator) {
    auto* bdm = static_cast<BindToDeviceMutator*>(mutator);
    int ret = setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, bdm->ifname.c_str(), bdm->ifname.size());
    if (ret != 0) {
        PLOG(ERROR) << "Can't bind socket to interface " << bdm->ifname;
        return false;
    }
    return true;
}

int Compare(grpc_socket_mutator* a, grpc_socket_mutator* b) {
    return ((a) < (b) ? -1 : ((a) > (b) ? 1 : 0));
}

void Destroy(grpc_socket_mutator* mutator) {
    auto* bdm = static_cast<BindToDeviceMutator*>(mutator);
    delete bdm;
}

constexpr grpc_socket_mutator_vtable kMutatorVtable = {
        .mutate_fd = MutateFd,
        .compare = Compare,
        .destroy = Destroy,
};

}  // namespace

grpc_socket_mutator* MakeBindToDeviceSocketMutator(std::string_view interface_name) {
    auto* bdm = new BindToDeviceMutator;
    grpc_socket_mutator_init(bdm, &kMutatorVtable);
    bdm->ifname = interface_name;
    return bdm;
}

}  // namespace android::hardware::automotive::remoteaccess
