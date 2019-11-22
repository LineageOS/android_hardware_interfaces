/*
 * Copyright (C) 2018 The Android Open Source Project
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

#define LOG_TAG "CommConn"

#include <thread>

#include <android/hardware/automotive/vehicle/2.0/IVehicle.h>
#include <log/log.h>

#include "CommConn.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

void CommConn::start() {
    mReadThread = std::make_unique<std::thread>(std::bind(&CommConn::readThread, this));
}

void CommConn::stop() {
    if (mReadThread->joinable()) {
        mReadThread->join();
    }
}

void CommConn::sendMessage(vhal_proto::EmulatorMessage const& msg) {
    int numBytes = msg.ByteSize();
    std::vector<uint8_t> buffer(static_cast<size_t>(numBytes));
    if (!msg.SerializeToArray(buffer.data(), numBytes)) {
        ALOGE("%s: SerializeToString failed!", __func__);
        return;
    }

    write(buffer);
}

void CommConn::readThread() {
    std::vector<uint8_t> buffer;
    while (isOpen()) {
        buffer = read();
        if (buffer.size() == 0) {
            ALOGI("%s: Read returned empty message, exiting read loop.", __func__);
            break;
        }

        vhal_proto::EmulatorMessage rxMsg;
        if (rxMsg.ParseFromArray(buffer.data(), static_cast<int32_t>(buffer.size()))) {
            vhal_proto::EmulatorMessage respMsg;
            mMessageProcessor->processMessage(rxMsg, respMsg);

            sendMessage(respMsg);
        }
    }
}

}  // namespace impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
