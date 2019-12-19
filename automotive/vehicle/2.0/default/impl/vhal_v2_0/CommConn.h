/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef android_hardware_automotive_vehicle_V2_0_impl_CommBase_H_
#define android_hardware_automotive_vehicle_V2_0_impl_CommBase_H_

#include <android/hardware/automotive/vehicle/2.0/IVehicle.h>
#include <string>
#include <thread>
#include <vector>

#include "VehicleHalProto.pb.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

/**
 * MessageProcess is an interface implemented by VehicleEmulator to process messages received
 * over a CommConn.
 */
class MessageProcessor {
   public:
    virtual ~MessageProcessor() = default;

    /**
     * Process a single message received over a CommConn. Populate the given respMsg with the reply
     * message we should send.
     */
    virtual void processMessage(vhal_proto::EmulatorMessage const& rxMsg,
                                vhal_proto::EmulatorMessage& respMsg) = 0;
};

/**
 * This is the interface that both PipeComm and SocketComm use to represent a connection. The
 * connection will listen for commands on a separate 'read' thread.
 */
class CommConn {
   public:
    CommConn(MessageProcessor* messageProcessor) : mMessageProcessor(messageProcessor) {}

    virtual ~CommConn() {}

    /**
     * Start the read thread reading messages from this connection.
     */
    virtual void start();

    /**
     * Closes a connection if it is open.
     */
    virtual void stop();

    /**
     * Returns true if the connection is open and available to send/receive.
     */
    virtual bool isOpen() = 0;

    /**
     * Blocking call to read data from the connection.
     *
     * @return std::vector<uint8_t> Serialized protobuf data received from emulator.  This will be
     *              an empty vector if the connection was closed or some other error occurred.
     */
    virtual std::vector<uint8_t> read() = 0;

    /**
     * Transmits a string of data to the emulator.
     *
     * @param data Serialized protobuf data to transmit.
     *
     * @return int Number of bytes transmitted, or -1 if failed.
     */
    virtual int write(const std::vector<uint8_t>& data) = 0;

    /**
     * Serialized and send the given message to the other side.
     */
    void sendMessage(vhal_proto::EmulatorMessage const& msg);

   protected:
    std::unique_ptr<std::thread> mReadThread;
    MessageProcessor* mMessageProcessor;

    /**
     * A thread that reads messages in a loop, and responds. You can stop this thread by calling
     * stop().
     */
    void readThread();
};

}  // namespace impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_V2_0_impl_CommBase_H_
