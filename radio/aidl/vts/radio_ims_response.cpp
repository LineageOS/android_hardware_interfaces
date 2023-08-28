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

#include "radio_ims_utils.h"

RadioImsResponse::RadioImsResponse(RadioServiceTest& parent) : parent_ims(parent) {}

ndk::ScopedAStatus RadioImsResponse::setSrvccCallInfoResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_ims.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioImsResponse::updateImsRegistrationInfoResponse(
        const RadioResponseInfo& info) {
    rspInfo = info;
    parent_ims.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioImsResponse::startImsTrafficResponse(const RadioResponseInfo& info,
        const std::optional<ConnectionFailureInfo>& response) {
    rspInfo = info;
    startImsTrafficResp = response;
    parent_ims.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioImsResponse::stopImsTrafficResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_ims.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioImsResponse::triggerEpsFallbackResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_ims.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioImsResponse::sendAnbrQueryResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_ims.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioImsResponse::updateImsCallStatusResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_ims.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}
