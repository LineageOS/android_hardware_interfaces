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

#include <aidl/android/hardware/net/nlinterceptor/IInterceptor.h>
#include <android-base/logging.h>
#include <android-base/macros.h>
#include <android/binder_manager.h>
#include <libnlinterceptor/libnlinterceptor.h>
#include <linux/netlink.h>

#include <mutex>

namespace android::nlinterceptor {
using namespace std::string_literals;
using namespace ::aidl::android::hardware::net::nlinterceptor;
using base::borrowed_fd;
using AidlInterceptedSocket =
    ::aidl::android::hardware::net::nlinterceptor::InterceptedSocket;

static const auto kServiceName = IInterceptor::descriptor + "/default"s;

InterceptedSocket::InterceptedSocket(
    ::aidl::android::hardware::net::nlinterceptor::InterceptedSocket sock)
    : nlFamily(sock.nlFamily), portId(sock.portId) {}

InterceptedSocket::InterceptedSocket(uint32_t nlFamily, uint32_t portId)
    : nlFamily(nlFamily), portId(portId) {}

std::ostream& operator<<(std::ostream& os, const InterceptedSocket& sock) {
    return os << "family: " << sock.nlFamily << ", portId: " << sock.portId;
}

bool InterceptedSocket::operator<(const InterceptedSocket& other) const {
    if (nlFamily != other.nlFamily) {
        return nlFamily < other.nlFamily;
    }
    return portId < other.portId;
}

InterceptedSocket::operator sockaddr_nl() const {
    return {
        .nl_family = AF_NETLINK,
        .nl_pad = 0,
        .nl_pid = portId,
        .nl_groups = 0,
    };
}

InterceptedSocket::operator AidlInterceptedSocket() const {
    return {
        .nlFamily = static_cast<int32_t>(nlFamily),
        .portId = static_cast<int32_t>(portId),
    };
}

bool isEnabled() {
    static std::mutex supportedMutex;
    static std::optional<bool> interceptorSupported;
    // Avoid querying service manager when we can cache the result.
    if (interceptorSupported.has_value()) return *interceptorSupported;
    std::lock_guard lock(supportedMutex);
    if (interceptorSupported.has_value()) return *interceptorSupported;

    if (!AServiceManager_isDeclared(kServiceName.c_str())) {
        interceptorSupported = false;
        return false;
    }
    interceptorSupported = true;
    return true;
}

static IInterceptor& getInstance() {
    static std::mutex instanceMutex;
    static std::shared_ptr<IInterceptor> interceptorInstance;
    CHECK(isEnabled()) << "Can't getInstance! Interceptor not supported!";
    // Don't overwrite the pointer once we've acquired it.
    if (interceptorInstance != nullptr) return *interceptorInstance;
    std::lock_guard lock(instanceMutex);
    if (interceptorInstance != nullptr) return *interceptorInstance;
    interceptorInstance = IInterceptor::fromBinder(
        ndk::SpAIBinder(AServiceManager_waitForService(kServiceName.c_str())));
    CHECK(interceptorInstance != nullptr)
        << "Failed to get Netlink Interceptor service!";
    return *interceptorInstance;
}

std::optional<InterceptedSocket> createSocket(borrowed_fd clientSocket,
                                              const std::string& clientName) {
    sockaddr_nl nladdr = {};
    socklen_t nlsize = sizeof(nladdr);
    if (getsockname(clientSocket.get(), reinterpret_cast<sockaddr*>(&nladdr),
                    &nlsize) < 0) {
        PLOG(ERROR) << "Failed to get pid of fd passed by " << clientName;
        return std::nullopt;
    }

    ::aidl::android::hardware::net::nlinterceptor::InterceptedSocket
        interceptedSocket;
    auto aidlStatus = getInstance().createSocket(
        nladdr.nl_family, nladdr.nl_pid, clientName, &interceptedSocket);
    if (!aidlStatus.isOk()) {
        return std::nullopt;
    }

    return InterceptedSocket{nladdr.nl_family,
                             uint32_t(interceptedSocket.portId)};
}

void closeSocket(const InterceptedSocket& sock) {
    auto aidlStatus = getInstance().closeSocket(sock);
    if (!aidlStatus.isOk()) {
        LOG(ERROR) << "Failed to close socket with pid = " << sock.portId;
    }
}

bool subscribe(const InterceptedSocket& sock, uint32_t group) {
    auto aidlStatus = getInstance().subscribeGroup(sock, group);
    return aidlStatus.isOk();
}

bool unsubscribe(const InterceptedSocket& sock, uint32_t group) {
    auto aidlStatus = getInstance().unsubscribeGroup(sock, group);
    return aidlStatus.isOk();
}

extern "C" bool android_nlinterceptor_isEnabled() { return isEnabled(); }

extern "C" bool android_nlinterceptor_createSocket(
    int clientSocketFd, const char* clientName,
    android_nlinterceptor_InterceptedSocket* interceptedSocket) {
    if (!clientName || clientSocketFd <= 0) return false;
    const auto maybeSocket =
        createSocket(borrowed_fd(clientSocketFd), clientName);
    if (!maybeSocket) return false;
    *interceptedSocket = {.nlFamily = maybeSocket->nlFamily,
                          .portId = maybeSocket->portId};
    return true;
}

extern "C" void android_nlinterceptor_closeSocket(android_nlinterceptor_InterceptedSocket sock) {
    closeSocket({sock.nlFamily, sock.portId});
}

extern "C" bool android_nlinterceptor_subscribe(android_nlinterceptor_InterceptedSocket sock,
                                                uint32_t group) {
    return subscribe({sock.nlFamily, sock.portId}, group);
}

extern "C" bool android_nlinterceptor_unsubscribe(android_nlinterceptor_InterceptedSocket sock,
                                                  uint32_t group) {
    return unsubscribe({sock.nlFamily, sock.portId}, group);
}

}  // namespace android::nlinterceptor
