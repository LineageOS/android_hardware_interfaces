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
#pragma once

#include "GuaranteedCallback.h"

#include <aidl/android/hardware/radio/sap/ISapCallback.h>
#include <android/hardware/radio/1.0/ISapCallback.h>

namespace android::hardware::radio::compat {

class SapCallback : public V1_0::ISapCallback {
    GuaranteedCallback<aidl::android::hardware::radio::sap::ISapCallback,
                       aidl::android::hardware::radio::sap::ISapCallbackDefault>
            mCallback;

    Return<void> apduResponse(int32_t serial, V1_0::SapResultCode resultCode,
                              const ::android::hardware::hidl_vec<uint8_t>& apduRsp) override;
    Return<void> connectResponse(int32_t serial, V1_0::SapConnectRsp sapConnectRsp,
                                 int32_t maxMsgSize) override;
    Return<void> disconnectIndication(int32_t serial,
                                      V1_0::SapDisconnectType disconnectType) override;
    Return<void> disconnectResponse(int32_t serial) override;
    Return<void> errorResponse(int32_t serial) override;
    Return<void> powerResponse(int32_t serial, V1_0::SapResultCode resultCode) override;
    Return<void> resetSimResponse(int32_t serial, V1_0::SapResultCode resultCode) override;
    Return<void> statusIndication(int32_t serial, V1_0::SapStatus status) override;
    Return<void> transferAtrResponse(int32_t serial, V1_0::SapResultCode resultCode,
                                     const ::android::hardware::hidl_vec<uint8_t>& atr) override;
    Return<void> transferCardReaderStatusResponse(int32_t serial, V1_0::SapResultCode resultCode,
                                                  int32_t cardReaderStatus) override;
    Return<void> transferProtocolResponse(int32_t serial, V1_0::SapResultCode resultCode) override;

  public:
    void setResponseFunction(
            const std::shared_ptr<aidl::android::hardware::radio::sap::ISapCallback>& callback);

    std::shared_ptr<aidl::android::hardware::radio::sap::ISapCallback> respond();
};

}  // namespace android::hardware::radio::compat
