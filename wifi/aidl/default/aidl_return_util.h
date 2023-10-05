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

#ifndef AIDL_RETURN_UTIL_H_
#define AIDL_RETURN_UTIL_H_

#include "aidl_sync_util.h"
#include "wifi_status_util.h"

namespace aidl {
namespace android {
namespace hardware {
namespace wifi {
namespace aidl_return_util {
using aidl::android::hardware::wifi::WifiStatusCode;
using aidl::android::hardware::wifi::aidl_sync_util::acquireGlobalLock;

/**
 * These utility functions are used to invoke a method on the provided
 * AIDL interface object.
 * These functions checks if the provided AIDL interface object is valid.
 * a) If valid, Invokes the corresponding internal implementation function of
 * the AIDL method.
 * b) If invalid, return without calling the internal implementation function.
 */

// Use for AIDL methods which return only an AIDL status.
template <typename ObjT, typename WorkFuncT, typename... Args>
::ndk::ScopedAStatus validateAndCall(ObjT* obj, WifiStatusCode status_code_if_invalid,
                                     WorkFuncT&& work, Args&&... args) {
    const auto lock = acquireGlobalLock();
    if (obj->isValid()) {
        return (obj->*work)(std::forward<Args>(args)...);
    } else {
        return createWifiStatus(status_code_if_invalid);
    }
}

// Use for AIDL methods which return only an AIDL status.
// This version passes the global lock acquired to the body of the method.
template <typename ObjT, typename WorkFuncT, typename... Args>
::ndk::ScopedAStatus validateAndCallWithLock(ObjT* obj, WifiStatusCode status_code_if_invalid,
                                             WorkFuncT&& work, Args&&... args) {
    auto lock = acquireGlobalLock();
    if (obj->isValid()) {
        return (obj->*work)(&lock, std::forward<Args>(args)...);
    } else {
        return createWifiStatus(status_code_if_invalid);
    }
}

// Use for AIDL methods which have a return value along with the AIDL status
template <typename ObjT, typename WorkFuncT, typename ReturnT, typename... Args>
::ndk::ScopedAStatus validateAndCall(ObjT* obj, WifiStatusCode status_code_if_invalid,
                                     WorkFuncT&& work, ReturnT* ret_val, Args&&... args) {
    const auto lock = acquireGlobalLock();
    if (obj->isValid()) {
        auto call_pair = (obj->*work)(std::forward<Args>(args)...);
        *ret_val = call_pair.first;
        return std::forward<::ndk::ScopedAStatus>(call_pair.second);
    } else {
        return ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(status_code_if_invalid));
    }
}

}  // namespace aidl_return_util
}  // namespace wifi
}  // namespace hardware
}  // namespace android
}  // namespace aidl
#endif  // AIDL_RETURN_UTIL_H_
