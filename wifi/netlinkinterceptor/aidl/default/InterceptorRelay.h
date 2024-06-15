/*
 * Copyright (C) 2021 The Android Open Source Project
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

#pragma once

#include <libnl++/Socket.h>

#include <atomic>
#include <mutex>
#include <thread>

namespace android::nlinterceptor {

class InterceptorRelay {
   public:
    /**
     * Wrapper around the netlink socket and thread which relays messages.
     *
     * \param nlFamily - netlink family to use for the netlink socket.
     * \param clientNlPid - pid of the client netlink socket.
     * \param clientName - name of the client to be used for debugging.
     */
    InterceptorRelay(uint32_t nlFamily, uint32_t clientNlPid,
                     const std::string& clientName);

    /**
     * Stops the relay thread if running and destroys itself.
     */
    ~InterceptorRelay();

    /**
     * Returns the PID of the internal Netlink socket.
     *
     * \return value of PID,
     */
    uint32_t getPid();

    /**
     * Spawns relay thread.
     */
    bool start();

    /**
     * Subscribes the internal socket to a single Netlink multicast group.
     *
     * \param nlGroup - Netlink group to subscribe to.
     * \returns - true for success, false for failure.
     */
    bool subscribeGroup(uint32_t nlGroup);

    /**
     * Unsubscribes the internal socket from a single Netlink multicast group.
     *
     * \param nlGroup - Netlink group to unsubscribe from.
     * \returns - true for success, false for failure.
     */
    bool unsubscribeGroup(uint32_t nlGroup);

   private:
    std::string mClientName;  ///< Name of client (Wificond, for example).
    std::optional<nl::Socket> mNlSocket;
    const uint32_t mClientNlPid = 0;  ///< pid of client NL socket.

    /**
     * If set to true, the relay thread should be running. Setting this to false
     * stops the relay thread.
     */
    std::atomic_bool mRunning = false;

    /**
     * Reads incoming Netlink messages destined for mNlSocket. If from the
     * kernel, the message is relayed to the client specified in the
     * constructor. Otherwise, the message is relayed to the kernel. This will
     * run as long as mRunning is set to true.
     */
    void relayMessages();

    std::thread mRelayThread;
};

}  // namespace android::nlinterceptor
