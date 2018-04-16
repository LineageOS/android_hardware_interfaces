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
#ifndef ANDROID_HARDWARE_RADIO_V1_2_SAP_H
#define ANDROID_HARDWARE_RADIO_V1_2_SAP_H

#include <android/hardware/radio/1.2/ISap.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace radio {
namespace V1_2 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct Sap : public ISap {
    sp<::android::hardware::radio::V1_0::ISapCallback> mSapCallback;
    // Methods from ::android::hardware::radio::V1_0::ISap follow.
    Return<void> setCallback(
        const sp<::android::hardware::radio::V1_0::ISapCallback>& sapCallback) override;
    Return<void> connectReq(int32_t token, int32_t maxMsgSize) override;
    Return<void> disconnectReq(int32_t token) override;
    Return<void> apduReq(int32_t token, ::android::hardware::radio::V1_0::SapApduType type,
                         const hidl_vec<uint8_t>& command) override;
    Return<void> transferAtrReq(int32_t token) override;
    Return<void> powerReq(int32_t token, bool state) override;
    Return<void> resetSimReq(int32_t token) override;
    Return<void> transferCardReaderStatusReq(int32_t token) override;
    Return<void> setTransferProtocolReq(
        int32_t token,
        ::android::hardware::radio::V1_0::SapTransferProtocol transferProtocol) override;
};

}  // namespace implementation
}  // namespace V1_2
}  // namespace radio
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_RADIO_V1_2_SAP_H
