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

#ifndef android_hardware_automotive_vehicle_V2_0_impl_SocketComm_H_
#define android_hardware_automotive_vehicle_V2_0_impl_SocketComm_H_

#include <mutex>
#include <thread>
#include <vector>
#include "CommConn.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

class SocketConn;

/**
 * SocketComm opens a socket, and listens for connections from clients. Typically the client will be
 * adb's TCP port-forwarding to enable a host PC to connect to the VehicleHAL.
 */
class SocketComm {
   public:
    SocketComm(MessageProcessor* messageProcessor);
    virtual ~SocketComm();

    void start();
    void stop();

    /**
     * Serialized and send the given message to all connected clients.
     */
    void sendMessage(vhal_proto::EmulatorMessage const& msg);

   private:
    int mListenFd;
    std::unique_ptr<std::thread> mListenThread;
    std::vector<std::unique_ptr<SocketConn>> mOpenConnections;
    MessageProcessor* mMessageProcessor;
    std::mutex mMutex;

    /**
     * Opens the socket and begins listening.
     *
     * @return bool Returns true on success.
     */
    bool listen();

    /**
     * Blocks and waits for a connection from a client, returns a new SocketConn with the connection
     * or null, if the connection has been closed.
     *
     * @return int Returns fd or socket number if connection is successful.
     *              Otherwise, returns -1 if no connection is availble.
     */
    SocketConn* accept();

    void listenThread();

    void removeClosedConnections();
};

/**
 * SocketConn represents a single connection to a client.
 */
class SocketConn : public CommConn {
   public:
    SocketConn(MessageProcessor* messageProcessor, int sfd);
    virtual ~SocketConn() = default;

    /**
     * Blocking call to read data from the connection.
     *
     * @return std::vector<uint8_t> Serialized protobuf data received from emulator.  This will be
     *              an empty vector if the connection was closed or some other error occurred.
     */
    std::vector<uint8_t> read() override;

    /**
     * Closes a connection if it is open.
     */
    void stop() override;

    /**
     * Transmits a string of data to the emulator.
     *
     * @param data Serialized protobuf data to transmit.
     *
     * @return int Number of bytes transmitted, or -1 if failed.
     */
    int write(const std::vector<uint8_t>& data) override;

    inline bool isOpen() override { return mSockFd > 0; }

   private:
    int mSockFd;
};

}  // impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android


#endif  // android_hardware_automotive_vehicle_V2_0_impl_SocketComm_H_
