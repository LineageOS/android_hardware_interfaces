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
#include "radio_sap_utils.h"

#include <android-base/logging.h>

SapCallback::SapCallback(SapTest& parent) : parent_sap(parent) {}

::ndk::ScopedAStatus SapCallback::apduResponse(int32_t serialNumber, SapResultCode resultCode,
                                               const std::vector<uint8_t>& /*apduRsp*/) {
    sapResponseSerial = serialNumber;
    sapResultCode = resultCode;
    parent_sap.notify(serialNumber);
    return ndk::ScopedAStatus::ok();
}
::ndk::ScopedAStatus SapCallback::connectResponse(int32_t serialNumber,
                                                  SapConnectRsp /*sapConnectRsp*/,
                                                  int32_t /*maxMsgSize*/) {
    sapResponseSerial = serialNumber;
    parent_sap.notify(serialNumber);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus SapCallback::disconnectIndication(int32_t /*serialNumber*/,
                                                       SapDisconnectType /*sapDisconnectType*/) {
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus SapCallback::disconnectResponse(int32_t serialNumber) {
    sapResponseSerial = serialNumber;
    parent_sap.notify(serialNumber);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus SapCallback::errorResponse(int32_t /*serialNumber*/) {
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus SapCallback::powerResponse(int32_t serialNumber, SapResultCode resultCode) {
    sapResponseSerial = serialNumber;
    sapResultCode = resultCode;
    parent_sap.notify(serialNumber);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus SapCallback::resetSimResponse(int32_t serialNumber, SapResultCode resultCode) {
    sapResponseSerial = serialNumber;
    sapResultCode = resultCode;
    parent_sap.notify(serialNumber);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus SapCallback::statusIndication(int32_t /*serialNumber*/,
                                                   SapStatus /*sapStatus*/) {
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus SapCallback::transferAtrResponse(int32_t serialNumber,
                                                      SapResultCode resultCode,
                                                      const std::vector<uint8_t>& /*atr*/) {
    sapResponseSerial = serialNumber;
    sapResultCode = resultCode;
    parent_sap.notify(serialNumber);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus SapCallback::transferCardReaderStatusResponse(int32_t serialNumber,
                                                                   SapResultCode resultCode,
                                                                   int32_t /*cardReaderStatus*/) {
    sapResponseSerial = serialNumber;
    sapResultCode = resultCode;
    parent_sap.notify(serialNumber);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus SapCallback::transferProtocolResponse(int32_t serialNumber,
                                                           SapResultCode resultCode) {
    sapResponseSerial = serialNumber;
    sapResultCode = resultCode;
    parent_sap.notify(serialNumber);
    return ndk::ScopedAStatus::ok();
}
