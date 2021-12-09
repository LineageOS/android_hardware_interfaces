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

#include <aidl/android/hardware/net/nlinterceptor/BnInterceptor.h>
#include <libnlinterceptor/libnlinterceptor.h>

#include <map>

#include "InterceptorRelay.h"

namespace android::nlinterceptor {

class NetlinkInterceptor
    : public ::aidl::android::hardware::net::nlinterceptor::BnInterceptor {
    using ClientMap =
        std::map<::android::nlinterceptor::InterceptedSocket,
                 std::unique_ptr<::android::nlinterceptor::InterceptorRelay>>;

    using AidlInterceptedSocket =
        ::aidl::android::hardware::net::nlinterceptor::InterceptedSocket;

   public:
    ndk::ScopedAStatus createSocket(
        int32_t nlFamily, int32_t clientNlPid, const std::string& clientName,
        AidlInterceptedSocket* interceptedSocket) override;

    ndk::ScopedAStatus closeSocket(
        const AidlInterceptedSocket& interceptedSocket) override;

    ndk::ScopedAStatus subscribeGroup(
        const AidlInterceptedSocket& interceptedSocket,
        int32_t nlGroup) override;

    ndk::ScopedAStatus unsubscribeGroup(
        const AidlInterceptedSocket& interceptedSocket,
        int32_t nlGroup) override;

   private:
    ClientMap mClientMap;
};

}  // namespace android::nlinterceptor
