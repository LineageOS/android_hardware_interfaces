/*
 * Copyright (C) 2022 The Android Open Source Project
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

#define LOG_TAG "RemoteAccessImpl"

#include "RemoteAccessService.h"

#include "BindToDeviceSocketMutator.h"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <grpcpp/create_channel.h>
#include <libnetdevice/libnetdevice.h>
#include <stdlib.h>

constexpr char SERVICE_NAME[] = "android.hardware.automotive.remoteaccess.IRemoteAccess/default";

int main(int /* argc */, char* /* argv */[]) {
#ifndef GRPC_SERVICE_ADDRESS
    LOG(ERROR) << "GRPC_SERVICE_ADDRESS is not defined, exiting";
    exit(1);
#endif
    LOG(INFO) << "Registering RemoteAccessService as service, server: " << GRPC_SERVICE_ADDRESS
              << "...";
    grpc::ChannelArguments grpcargs = {};

#ifdef GRPC_SERVICE_IFNAME
    grpcargs.SetSocketMutator(
            android::hardware::automotive::remoteaccess::MakeBindToDeviceSocketMutator(
                    GRPC_SERVICE_IFNAME));
    LOG(DEBUG) << "GRPC_SERVICE_IFNAME specified as: " << GRPC_SERVICE_IFNAME;
    LOG(INFO) << "Waiting for interface: " << GRPC_SERVICE_IFNAME;
    android::netdevice::waitFor({GRPC_SERVICE_IFNAME},
                                android::netdevice::WaitCondition::PRESENT_AND_UP);
    LOG(INFO) << "Waiting for interface: " << GRPC_SERVICE_IFNAME << " done";
#endif
    auto channel = grpc::CreateChannel(GRPC_SERVICE_ADDRESS, grpc::InsecureChannelCredentials());
    auto clientStub = android::hardware::automotive::remoteaccess::WakeupClient::NewStub(channel);
    auto service = ndk::SharedRefBase::make<
            android::hardware::automotive::remoteaccess::RemoteAccessService>(clientStub.get());

    binder_exception_t err = AServiceManager_addService(service->asBinder().get(), SERVICE_NAME);
    if (err != EX_NONE) {
        LOG(ERROR) << "failed to register android.hardware.automotive.remote.IRemoteAccess service"
                   << ", exception: " << err;
        exit(1);
    }

    if (!ABinderProcess_setThreadPoolMaxThreadCount(1)) {
        LOG(ERROR) << "failed to set thread pool max thread count";
        exit(1);
    }
    ABinderProcess_startThreadPool();

    LOG(INFO) << "RemoteAccess service Ready";

    ABinderProcess_joinThreadPool();

    LOG(ERROR) << "Should not reach here";

    return 0;
}
