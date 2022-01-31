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

#include "radio_data_utils.h"

RadioDataResponse::RadioDataResponse(RadioServiceTest& parent) : parent_data(parent) {}

ndk::ScopedAStatus RadioDataResponse::acknowledgeRequest(int32_t /*serial*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioDataResponse::allocatePduSessionIdResponse(const RadioResponseInfo& info,
                                                                   int32_t id) {
    rspInfo = info;
    allocatedPduSessionId = id;
    parent_data.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioDataResponse::cancelHandoverResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_data.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioDataResponse::deactivateDataCallResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_data.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioDataResponse::getDataCallListResponse(
        const RadioResponseInfo& info, const std::vector<SetupDataCallResult>& /*dcResponse*/) {
    rspInfo = info;
    parent_data.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioDataResponse::getSlicingConfigResponse(
        const RadioResponseInfo& info, const SlicingConfig& /*slicingConfig*/) {
    rspInfo = info;
    parent_data.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioDataResponse::releasePduSessionIdResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_data.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioDataResponse::setDataAllowedResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_data.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioDataResponse::setDataProfileResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_data.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioDataResponse::setDataThrottlingResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_data.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioDataResponse::setInitialAttachApnResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_data.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioDataResponse::setupDataCallResponse(const RadioResponseInfo& info,
                                                            const SetupDataCallResult& dcResponse) {
    rspInfo = info;
    setupDataCallResult = dcResponse;
    parent_data.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioDataResponse::startHandoverResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_data.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioDataResponse::startKeepaliveResponse(const RadioResponseInfo& info,
                                                             const KeepaliveStatus& /*status*/) {
    rspInfo = info;
    parent_data.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioDataResponse::stopKeepaliveResponse(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_data.notify(info.serial);
    return ndk::ScopedAStatus::ok();
}
