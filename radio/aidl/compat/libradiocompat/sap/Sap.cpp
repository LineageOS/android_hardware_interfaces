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

#include <libradiocompat/Sap.h>

#include "commonStructs.h"
#include "debug.h"
#include "structs.h"

#include "collections.h"

#define RADIO_MODULE "Sap"

namespace android::hardware::radio::compat {

using ::ndk::ScopedAStatus;
namespace aidl = ::aidl::android::hardware::radio::sap;
constexpr auto ok = &ScopedAStatus::ok;

Sap::Sap(sp<V1_0::ISap> hidlHal) : mHal(hidlHal), mSapCallback(sp<SapCallback>::make()) {}

ScopedAStatus Sap::apduReq(int32_t serial, aidl::SapApduType type,
                           const std::vector<uint8_t>& command) {
    LOG_CALL << serial;
    mHal->apduReq(serial, toHidl(type), toHidl(command));
    return ok();
}

ScopedAStatus Sap::connectReq(int32_t serial, int32_t maxMsgSize) {
    LOG_CALL << serial;
    mHal->connectReq(serial, maxMsgSize);
    return ok();
}

ScopedAStatus Sap::disconnectReq(int32_t serial) {
    LOG_CALL << serial;
    mHal->disconnectReq(serial);
    return ok();
}

ScopedAStatus Sap::powerReq(int32_t serial, bool state) {
    LOG_CALL << serial;
    mHal->powerReq(serial, state);
    return ok();
}

ScopedAStatus Sap::resetSimReq(int32_t serial) {
    LOG_CALL << serial;
    mHal->resetSimReq(serial);
    return ok();
}

ScopedAStatus Sap::setCallback(
        const std::shared_ptr<::aidl::android::hardware::radio::sap::ISapCallback>& sapCallback) {
    LOG_CALL << sapCallback;

    CHECK(sapCallback);

    mSapCallback->setResponseFunction(sapCallback);
    mHal->setCallback(mSapCallback).assertOk();
    return ok();
}
ScopedAStatus Sap::setTransferProtocolReq(int32_t serial,
                                          aidl::SapTransferProtocol transferProtocol) {
    LOG_CALL << serial;
    mHal->setTransferProtocolReq(serial, toHidl(transferProtocol));
    return ok();
}

ScopedAStatus Sap::transferAtrReq(int32_t serial) {
    LOG_CALL << serial;
    mHal->transferAtrReq(serial);
    return ok();
}
ScopedAStatus Sap::transferCardReaderStatusReq(int32_t serial) {
    LOG_CALL << serial;
    mHal->transferCardReaderStatusReq(serial);
    return ok();
}

}  // namespace android::hardware::radio::compat
