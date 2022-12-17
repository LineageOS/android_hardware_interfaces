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

#include "structs.h"

namespace android::hardware::radio::compat {

namespace aidl = ::aidl::android::hardware::radio::sap;

V1_0::SapApduType toHidl(const aidl::SapApduType sapApduType) {
    return V1_0::SapApduType(sapApduType);
}

V1_0::SapTransferProtocol toHidl(const aidl::SapTransferProtocol sapTransferProtocol) {
    return V1_0::SapTransferProtocol(sapTransferProtocol);
}

aidl::SapResultCode toAidl(const V1_0::SapResultCode sapResultCode) {
    return aidl::SapResultCode(sapResultCode);
}

aidl::SapConnectRsp toAidl(const V1_0::SapConnectRsp sapConnectRsp) {
    return aidl::SapConnectRsp(sapConnectRsp);
}

aidl::SapDisconnectType toAidl(const V1_0::SapDisconnectType sapDisconnectType) {
    return aidl::SapDisconnectType(sapDisconnectType);
}

aidl::SapStatus toAidl(const V1_0::SapStatus sapStatus) {
    return aidl::SapStatus(sapStatus);
}

}  // namespace android::hardware::radio::compat
