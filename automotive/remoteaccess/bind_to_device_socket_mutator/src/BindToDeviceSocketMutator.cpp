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
#include <sys/socket.h>

namespace android::hardware::automotive::remoteaccess {

bool BindToDeviceSocketMutator::mutateFd(int fd) {
    int ret = setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, mIfname.c_str(), mIfname.size());
    if (ret != 0) {
        PLOG(ERROR) << "Can't bind socket to interface " << mIfname;
        return false;
    }
    return true;
}

bool bind_to_device_mutator_mutate_fd(int fd, grpc_socket_mutator* mutator) {
    BindToDeviceSocketMutator* bsm = (BindToDeviceSocketMutator*)mutator;
    return bsm->mutateFd(fd);
}

int bind_to_device_mutator_compare(grpc_socket_mutator* a, grpc_socket_mutator* b) {
    return ((a) < (b) ? -1 : ((a) > (b) ? 1 : 0));
}

void bind_to_device_mutator_destroy(grpc_socket_mutator* mutator) {
    BindToDeviceSocketMutator* bsm = (BindToDeviceSocketMutator*)mutator;
    delete bsm;
}

grpc_socket_mutator_vtable bind_to_device_mutator_vtable = {bind_to_device_mutator_mutate_fd,
                                                            bind_to_device_mutator_compare,
                                                            bind_to_device_mutator_destroy};

BindToDeviceSocketMutator::BindToDeviceSocketMutator(const std::string_view& interface_name) {
    mIfname = interface_name;
    grpc_socket_mutator_init(this, &bind_to_device_mutator_vtable);
}

}  // namespace android::hardware::automotive::remoteaccess
