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

namespace {

constexpr char GRPC_SERVICE_CONFIG_FILE[] = "/vendor/etc/automotive/powercontroller/serverconfig";
constexpr char SERVICE_NAME[] = "android.hardware.automotive.remoteaccess.IRemoteAccess/default";

void maybeGetGrpcServiceInfo(std::string* address, std::string* ifname) {
    std::ifstream ifs(GRPC_SERVICE_CONFIG_FILE);
    if (!ifs) {
        LOG(INFO) << "Cannot open grpc service config file at: " << GRPC_SERVICE_CONFIG_FILE
                  << ", assume no service is available";
        return;
    }
    int count = 0;
    while (ifs.good()) {
        std::string line;
        ifs >> line;
        // First line is address, second line, if present is ifname.
        if (count == 0) {
            *address = line;
        } else {
            *ifname = line;
            break;
        }
        count++;
    }
    ifs.close();
}

}  // namespace

int main(int /* argc */, char* /* argv */[]) {
    std::string grpcServiceAddress = "";
    std::string grpcServiceIfname = "";
    maybeGetGrpcServiceInfo(&grpcServiceAddress, &grpcServiceIfname);

    std::unique_ptr<android::hardware::automotive::remoteaccess::WakeupClient::Stub> grpcStub;

    if (grpcServiceAddress != "") {
        LOG(INFO) << "Registering RemoteAccessService as service, server: " << grpcServiceAddress
                  << "...";
        grpc::ChannelArguments grpcargs = {};

        if (grpcServiceIfname != "") {
            grpcargs.SetSocketMutator(
                    android::hardware::automotive::remoteaccess::MakeBindToDeviceSocketMutator(
                            grpcServiceIfname));
            LOG(DEBUG) << "grpcServiceIfname specified as: " << grpcServiceIfname;
            LOG(INFO) << "Waiting for interface: " << grpcServiceIfname;
            android::netdevice::waitFor({grpcServiceIfname},
                                        android::netdevice::WaitCondition::PRESENT_AND_UP);
            LOG(INFO) << "Waiting for interface: " << grpcServiceIfname << " done";
        }
        auto channel = grpc::CreateChannel(grpcServiceAddress, grpc::InsecureChannelCredentials());
        grpcStub = android::hardware::automotive::remoteaccess::WakeupClient::NewStub(channel);
    } else {
        LOG(INFO) << "grpcServiceAddress is not defined, work in fake mode";
    }

    auto service = ndk::SharedRefBase::make<
            android::hardware::automotive::remoteaccess::RemoteAccessService>(grpcStub.get());

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
