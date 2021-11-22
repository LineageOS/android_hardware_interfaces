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

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include "NetlinkInterceptor.h"

namespace android::nlinterceptor {
using namespace std::string_literals;

static void service() {
    base::SetDefaultTag("nlinterceptor");
    base::SetMinimumLogSeverity(base::VERBOSE);
    LOG(DEBUG) << "Netlink Interceptor service starting...";

    // TODO(202549296): Sometimes this causes an Address Sanitizer error.
    auto interceptor = ndk::SharedRefBase::make<NetlinkInterceptor>();
    const auto instance = NetlinkInterceptor::descriptor + "/default"s;
    const auto status = AServiceManager_addService(
        interceptor->asBinder().get(), instance.c_str());
    CHECK(status == STATUS_OK);

    ABinderProcess_joinThreadPool();
    LOG(FATAL) << "Netlink Interceptor has stopped";
}

}  // namespace android::nlinterceptor

int main() {
    ::android::nlinterceptor::service();
    return 0;
}
