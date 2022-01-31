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

#include "radio_messaging_utils.h"

RadioMessagingResponse::RadioMessagingResponse(RadioServiceTest& parent)
    : parent_messaging(parent) {}

ndk::ScopedAStatus RadioMessagingResponse::acknowledgeIncomingGsmSmsWithPduResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_messaging.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioMessagingResponse::acknowledgeLastIncomingCdmaSmsResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_messaging.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioMessagingResponse::acknowledgeLastIncomingGsmSmsResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_messaging.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioMessagingResponse::acknowledgeRequest(int32_t /*serial*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioMessagingResponse::deleteSmsOnRuimResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_messaging.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioMessagingResponse::deleteSmsOnSimResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_messaging.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioMessagingResponse::getCdmaBroadcastConfigResponse(
        const RadioResponseInfo& info, const std::vector<CdmaBroadcastSmsConfigInfo>& /*configs*/) {
    rspInfo = info;
    parent_messaging.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioMessagingResponse::getGsmBroadcastConfigResponse(
        const RadioResponseInfo& info, const std::vector<GsmBroadcastSmsConfigInfo>& /*configs*/) {
    rspInfo = info;
    parent_messaging.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioMessagingResponse::getSmscAddressResponse(const RadioResponseInfo& info,
                                                                  const std::string& /*smsc*/) {
    rspInfo = info;
    parent_messaging.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioMessagingResponse::reportSmsMemoryStatusResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_messaging.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioMessagingResponse::sendCdmaSmsExpectMoreResponse(
        const RadioResponseInfo& info, const SendSmsResult& sms) {
    rspInfo = info;
    sendSmsResult = sms;
    parent_messaging.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioMessagingResponse::sendCdmaSmsResponse(const RadioResponseInfo& info,
                                                               const SendSmsResult& sms) {
    rspInfo = info;
    sendSmsResult = sms;
    parent_messaging.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioMessagingResponse::sendImsSmsResponse(const RadioResponseInfo& info,
                                                              const SendSmsResult& /*sms*/) {
    rspInfo = info;
    parent_messaging.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioMessagingResponse::sendSmsExpectMoreResponse(const RadioResponseInfo& info,
                                                                     const SendSmsResult& sms) {
    rspInfo = info;
    sendSmsResult = sms;
    parent_messaging.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioMessagingResponse::sendSmsResponse(const RadioResponseInfo& info,
                                                           const SendSmsResult& sms) {
    rspInfo = info;
    sendSmsResult = sms;
    parent_messaging.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioMessagingResponse::setCdmaBroadcastActivationResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_messaging.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioMessagingResponse::setCdmaBroadcastConfigResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_messaging.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioMessagingResponse::setGsmBroadcastActivationResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_messaging.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioMessagingResponse::setGsmBroadcastConfigResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_messaging.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioMessagingResponse::setSmscAddressResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_messaging.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioMessagingResponse::writeSmsToRuimResponse(const RadioResponseInfo& info,
                                                                  int32_t /*index*/) {
    rspInfo = info;
    parent_messaging.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioMessagingResponse::writeSmsToSimResponse(const RadioResponseInfo& info,
                                                                 int32_t /*index*/) {
    rspInfo = info;
    parent_messaging.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}
