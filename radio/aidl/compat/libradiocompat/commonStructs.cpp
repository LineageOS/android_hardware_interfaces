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

#include "commonStructs.h"

namespace android::hardware::radio::compat {

namespace aidl = ::aidl::android::hardware::radio;

aidl::RadioResponseInfo notSupported(int32_t serial) {
    return {
            .type = aidl::RadioResponseType::SOLICITED,
            .serial = serial,
            .error = aidl::RadioError::REQUEST_NOT_SUPPORTED,
    };
}

std::string toAidl(const hidl_string& str) {
    return str;
}

hidl_string toHidl(const std::string& str) {
    return str;
}

uint8_t toAidl(int8_t v) {
    return v;
}

int8_t toAidl(uint8_t v) {
    return v;
}

int32_t toAidl(uint32_t v) {
    return v;
}

uint8_t toHidl(int8_t v) {
    return v;
}

aidl::RadioIndicationType toAidl(V1_0::RadioIndicationType type) {
    return aidl::RadioIndicationType(type);
}

aidl::RadioResponseType toAidl(V1_0::RadioResponseType type) {
    return aidl::RadioResponseType(type);
}

aidl::RadioError toAidl(V1_0::RadioError err) {
    return aidl::RadioError(err);
}

aidl::RadioError toAidl(V1_6::RadioError err) {
    return aidl::RadioError(err);
}

aidl::RadioResponseInfo toAidl(const V1_0::RadioResponseInfo& info) {
    return {
            .type = toAidl(info.type),
            .serial = info.serial,
            .error = toAidl(info.error),
    };
}

aidl::RadioResponseInfo toAidl(const V1_6::RadioResponseInfo& info) {
    return {
            .type = toAidl(info.type),
            .serial = info.serial,
            .error = toAidl(info.error),
    };
}

}  // namespace android::hardware::radio::compat
