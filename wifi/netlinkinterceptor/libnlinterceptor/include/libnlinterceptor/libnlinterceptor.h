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

#ifdef __cplusplus

#include <aidl/android/hardware/net/nlinterceptor/InterceptedSocket.h>
#include <android-base/unique_fd.h>
#include <linux/netlink.h>

#include <optional>
#include <string>

namespace android::nlinterceptor {

/**
 * Wrapper structure to uniquely identifies a socket that Netlink Interceptor
 * has allocated for us.
 */
struct InterceptedSocket {
    uint32_t nlFamily;
    uint32_t portId;

    InterceptedSocket(
        ::aidl::android::hardware::net::nlinterceptor::InterceptedSocket sock);
    InterceptedSocket(uint32_t nlFamily, uint32_t portId);

    bool operator<(const InterceptedSocket& other) const;
    operator sockaddr_nl() const;
    operator ::aidl::android::hardware::net::nlinterceptor::InterceptedSocket()
        const;
};

/**
 * Output stream operator for InterceptedSocket
 */
std::ostream& operator<<(std::ostream& os, const InterceptedSocket& sock);

/**
 * Checks if an instance Netlink Interceptor exists.
 *
 * \return true if supported, false if not.
 */
bool isEnabled();

/**
 * Asks Netlink Interceptor to allocate a socket to which we can send Netlink
 * traffic.
 *
 * \param clientSocket - File descriptor for the client's Netlink socket.
 * \param clientName - Human readable name of the client application.
 * \return Identifier for the socket created by Netlink Interceptor, nullopt on
 * error.
 */
std::optional<InterceptedSocket> createSocket(base::borrowed_fd clientSocket,
                                              const std::string& clientName);

/**
 * Asks Netlink Interceptor to close a socket that it created for us previously,
 * if it exists.
 *
 * \param sock - Identifier for the socket created by Netlink Interceptor.
 */
void closeSocket(const InterceptedSocket& sock);

/**
 * Asks Netlink Interceptor to subscribe a socket that it created for us
 * previously to a specified multicast group.
 *
 * \param sock - Identifier for the socket created by Netlink Interceptor.
 * \param group - A single Netlink multicast group for which we would like to
 * receive events.
 * \return true for success, false if something went wrong.
 */
bool subscribe(const InterceptedSocket& sock, uint32_t group);

/**
 * Asks Netlink Interceptor to unsubscribe a socket that it created for us
 * previously from a specified multicast group.
 *
 * \param sock - Identifier for the socket created by Netlink Interceptor.
 * \param group - A single Netlink multicast group for which we no longer wish
 * to receive events.
 * \return true for success, false if something went wrong.
 */
bool unsubscribe(const InterceptedSocket& sock, uint32_t group);
}  // namespace android::nlinterceptor
#endif

#ifdef __cplusplus
extern "C" {
#endif

// C wrappers for libnlinterceptor
struct android_nlinterceptor_InterceptedSocket {
    uint32_t nlFamily;
    uint32_t portId;
};

bool android_nlinterceptor_isEnabled();

bool android_nlinterceptor_createSocket(
    int clientSocketFd, const char* clientName,
    struct android_nlinterceptor_InterceptedSocket* interceptedSocket);

void android_nlinterceptor_closeSocket(struct android_nlinterceptor_InterceptedSocket sock);

bool android_nlinterceptor_subscribe(struct android_nlinterceptor_InterceptedSocket sock,
                                     uint32_t group);

bool android_nlinterceptor_unsubscribe(struct android_nlinterceptor_InterceptedSocket sock,
                                       uint32_t group);

#ifdef __cplusplus
}
#endif
