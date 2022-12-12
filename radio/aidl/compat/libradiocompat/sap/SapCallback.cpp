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

#include <libradiocompat/SapCallback.h>

#include "commonStructs.h"
#include "debug.h"
#include "structs.h"

#include "collections.h"

#define RADIO_MODULE "SapCallback"

namespace android::hardware::radio::compat {

namespace aidl = ::aidl::android::hardware::radio::sap;

void SapCallback::setResponseFunction(const std::shared_ptr<aidl::ISapCallback>& callback) {
    mCallback = callback;
}

std::shared_ptr<aidl::ISapCallback> SapCallback::respond() {
    return mCallback.get();
}

Return<void> SapCallback::apduResponse(int32_t serial, V1_0::SapResultCode resultCode,
                                       const hidl_vec<uint8_t>& apduRsp) {
    LOG_CALL << serial;
    respond()->apduResponse(serial, toAidl(resultCode), apduRsp);
    return {};
}

Return<void> SapCallback::connectResponse(int32_t serial, V1_0::SapConnectRsp sapConnectRsp,
                                          int32_t maxMsgSize) {
    LOG_CALL << serial;
    respond()->connectResponse(serial, toAidl(sapConnectRsp), maxMsgSize);
    return {};
}

Return<void> SapCallback::disconnectIndication(int32_t serial,
                                               V1_0::SapDisconnectType disconnectType) {
    LOG_CALL << serial;
    respond()->disconnectIndication(serial, toAidl(disconnectType));
    return {};
}

Return<void> SapCallback::disconnectResponse(int32_t serial) {
    LOG_CALL << serial;
    respond()->disconnectResponse(serial);
    return {};
}

Return<void> SapCallback::errorResponse(int32_t serial) {
    LOG_CALL << serial;
    respond()->errorResponse(serial);
    return {};
}

Return<void> SapCallback::powerResponse(int32_t serial, V1_0::SapResultCode resultCode) {
    LOG_CALL << serial;
    respond()->powerResponse(serial, toAidl(resultCode));
    return {};
}

Return<void> SapCallback::resetSimResponse(int32_t serial, V1_0::SapResultCode resultCode) {
    LOG_CALL << serial;
    respond()->resetSimResponse(serial, toAidl(resultCode));
    return {};
}

Return<void> SapCallback::statusIndication(int32_t serial, V1_0::SapStatus status) {
    LOG_CALL << serial;
    respond()->statusIndication(serial, toAidl(status));
    return {};
}

Return<void> SapCallback::transferAtrResponse(int32_t serial, V1_0::SapResultCode resultCode,
                                              const hidl_vec<uint8_t>& atr) {
    LOG_CALL << serial;
    respond()->transferAtrResponse(serial, toAidl(resultCode), atr);
    return {};
}

Return<void> SapCallback::transferCardReaderStatusResponse(int32_t serial,
                                                           V1_0::SapResultCode resultCode,
                                                           int32_t cardReaderStatus) {
    LOG_CALL << serial;
    respond()->transferCardReaderStatusResponse(serial, toAidl(resultCode), cardReaderStatus);
    return {};
}

Return<void> SapCallback::transferProtocolResponse(int32_t serial, V1_0::SapResultCode resultCode) {
    LOG_CALL << serial;
    respond()->transferProtocolResponse(serial, toAidl(resultCode));
    return {};
}

}  // namespace android::hardware::radio::compat
