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

#include "NetlinkInterceptor.h"

#include <android-base/logging.h>
#include <libnl++/Socket.h>

namespace android::nlinterceptor {

ndk::ScopedAStatus NetlinkInterceptor::createSocket(
    int32_t nlFamilyAidl, int32_t clientNlPidAidl,
    const std::string& clientName, AidlInterceptedSocket* interceptedSocket) {
    auto nlFamily = static_cast<uint32_t>(nlFamilyAidl);
    auto clientNlPid = static_cast<uint32_t>(clientNlPidAidl);
    uint32_t interceptorNlPid = 0;

    std::unique_ptr<InterceptorRelay> interceptor =
        std::make_unique<InterceptorRelay>(nlFamily, clientNlPid, clientName);

    interceptorNlPid = interceptor->getPid();

    if (interceptorNlPid == 0) {
        LOG(ERROR) << "Failed to create a Netlink socket for " << clientName
                   << ", " << nlFamily << ":" << clientNlPid;
        return ndk::ScopedAStatus(AStatus_fromStatus(::android::UNKNOWN_ERROR));
    }

    if (mClientMap.find({nlFamily, interceptorNlPid}) != mClientMap.end()) {
        LOG(ERROR) << "A socket with pid " << interceptorNlPid
                   << " already exists!";
        return ndk::ScopedAStatus(AStatus_fromStatus(::android::UNKNOWN_ERROR));
    }

    if (!interceptor->start()) {
        LOG(ERROR) << "Failed to start interceptor thread!";
        return ndk::ScopedAStatus(AStatus_fromStatus(::android::UNKNOWN_ERROR));
    }

    if (!mClientMap
             .emplace(InterceptedSocket(nlFamily, interceptorNlPid),
                      std::move(interceptor))
             .second) {
        // If this happens, it is very bad.
        LOG(FATAL) << "Failed to insert interceptor instance with pid "
                   << interceptorNlPid << " into map!";
        return ndk::ScopedAStatus(AStatus_fromStatus(::android::UNKNOWN_ERROR));
    }

    interceptedSocket->nlFamily = nlFamily;
    interceptedSocket->portId = interceptorNlPid;

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus NetlinkInterceptor::closeSocket(
    const AidlInterceptedSocket& interceptedSocket) {
    InterceptedSocket sock(interceptedSocket);

    auto interceptorIt = mClientMap.find(sock);
    if (interceptorIt == mClientMap.end()) {
        LOG(ERROR) << "closeSocket Failed! No such socket " << sock;
        return ndk::ScopedAStatus(AStatus_fromStatus(::android::UNKNOWN_ERROR));
    }
    mClientMap.erase(interceptorIt);

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus NetlinkInterceptor::subscribeGroup(
    const AidlInterceptedSocket& interceptedSocket, int32_t nlGroupAidl) {
    InterceptedSocket sock(interceptedSocket);
    auto nlGroup = static_cast<uint32_t>(nlGroupAidl);

    auto interceptorIt = mClientMap.find(sock);
    if (interceptorIt == mClientMap.end()) {
        LOG(ERROR) << "subscribeGroup failed! No such socket " << sock;
        return ndk::ScopedAStatus(AStatus_fromStatus(::android::UNKNOWN_ERROR));
    }

    auto& interceptor = interceptorIt->second;
    if (!interceptor->subscribeGroup(nlGroup)) {
        LOG(ERROR) << "Failed to subscribe " << sock << " to " << nlGroup;
        return ndk::ScopedAStatus(AStatus_fromStatus(::android::UNKNOWN_ERROR));
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus NetlinkInterceptor::unsubscribeGroup(
    const AidlInterceptedSocket& interceptedSocket, int32_t nlGroupAidl) {
    InterceptedSocket sock(interceptedSocket);
    auto nlGroup = static_cast<uint32_t>(nlGroupAidl);

    auto interceptorIt = mClientMap.find(sock);
    if (interceptorIt == mClientMap.end()) {
        LOG(ERROR) << "unsubscribeGroup failed! No such socket " << sock;
        return ndk::ScopedAStatus(AStatus_fromStatus(::android::UNKNOWN_ERROR));
    }

    auto& interceptor = interceptorIt->second;
    if (!interceptor->unsubscribeGroup(nlGroup)) {
        LOG(ERROR) << "Failed to unsubscribe " << sock << " from " << nlGroup;
        return ndk::ScopedAStatus(AStatus_fromStatus(::android::UNKNOWN_ERROR));
    }
    return ndk::ScopedAStatus::ok();
}
}  // namespace android::nlinterceptor
