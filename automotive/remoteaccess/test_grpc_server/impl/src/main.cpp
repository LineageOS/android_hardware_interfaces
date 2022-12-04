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

#include <string>

#include "TestWakeupClientServiceImpl.h"

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

using ::android::hardware::automotive::remoteaccess::TestWakeupClientServiceImpl;
using ::grpc::Server;
using ::grpc::ServerBuilder;
using ::grpc::ServerWriter;

void RunServer(const std::string& serviceAddr) {
    std::shared_ptr<TestWakeupClientServiceImpl> service =
            std::make_unique<TestWakeupClientServiceImpl>();

    ServerBuilder builder;
    builder.AddListeningPort(serviceAddr, grpc::InsecureServerCredentials());
    builder.RegisterService(service.get());
    std::unique_ptr<Server> server(builder.BuildAndStart());
    printf("Test Remote Access GRPC Server listening on %s\n", serviceAddr.c_str());
    server->Wait();
}

int main(int argc, char** argv) {
    std::string serviceAddr = GRPC_SERVICE_ADDRESS;
    if (argc > 1) {
        serviceAddr = argv[1];
    }
    RunServer(serviceAddr);
    return 0;
}
