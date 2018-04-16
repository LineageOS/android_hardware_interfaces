/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.1
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "Sap.h"

namespace android {
namespace hardware {
namespace radio {
namespace V1_2 {
namespace implementation {

// Methods from ::android::hardware::radio::V1_0::ISap follow.
Return<void> Sap::setCallback(
    const sp<::android::hardware::radio::V1_0::ISapCallback>& sapCallback) {
    mSapCallback = sapCallback;
    return Void();
}

Return<void> Sap::connectReq(int32_t /* token */, int32_t /* maxMsgSize */) {
    // TODO implement
    return Void();
}

Return<void> Sap::disconnectReq(int32_t /* token */) {
    // TODO implement
    return Void();
}

Return<void> Sap::apduReq(int32_t /* token */,
                          ::android::hardware::radio::V1_0::SapApduType /* type */,
                          const hidl_vec<uint8_t>& /* command */) {
    // TODO implement
    return Void();
}

Return<void> Sap::transferAtrReq(int32_t /* token */) {
    // TODO implement
    return Void();
}

Return<void> Sap::powerReq(int32_t /* token */, bool /* state */) {
    // TODO implement
    return Void();
}

Return<void> Sap::resetSimReq(int32_t /* token */) {
    // TODO implement
    return Void();
}

Return<void> Sap::transferCardReaderStatusReq(int32_t /* token */) {
    // TODO implement
    return Void();
}

Return<void> Sap::setTransferProtocolReq(
    int32_t /* token */,
    ::android::hardware::radio::V1_0::SapTransferProtocol /* transferProtocol */) {
    // TODO implement
    return Void();
}

}  // namespace implementation
}  // namespace V1_2
}  // namespace radio
}  // namespace hardware
}  // namespace android
