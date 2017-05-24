/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <radio_hidl_hal_utils_v1_1.h>

RadioResponse_v1_1::RadioResponse_v1_1(RadioHidlTest_v1_1& parent)
    : RadioResponse(parent), parent_v1_1(parent) {}

/* 1.1 Apis */
Return<void> RadioResponse_v1_1::setCarrierInfoForImsiEncryptionResponse(
    const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_1::setSimCardPowerResponse_1_1(const RadioResponseInfo& info) {
    rspInfo = info;
    parent_v1_1.notify();
    return Void();
}

Return<void> RadioResponse_v1_1::startNetworkScanResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}

Return<void> RadioResponse_v1_1::stopNetworkScanResponse(const RadioResponseInfo& /*info*/) {
    return Void();
}