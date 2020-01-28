/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include "RadioConfig.h"

namespace android {
namespace hardware {
namespace radio {
namespace config {
namespace V1_3 {
namespace implementation {

using namespace ::android::hardware::radio::V1_0;

// Methods from ::android::hardware::radio::config::V1_0::IRadioConfig follow.
Return<void> RadioConfig::setResponseFunctions(
        const sp<V1_0::IRadioConfigResponse>& radioConfigResponse,
        const sp<V1_0::IRadioConfigIndication>& radioConfigIndication) {
    mRadioConfigResponse = radioConfigResponse;
    mRadioConfigIndication = radioConfigIndication;

    mRadioConfigResponseV1_3 =
            V1_3::IRadioConfigResponse::castFrom(mRadioConfigResponse).withDefault(nullptr);
    mRadioConfigIndicationV1_3 =
            V1_3::IRadioConfigIndication::castFrom(mRadioConfigIndication).withDefault(nullptr);
    if (mRadioConfigResponseV1_3 == nullptr || mRadioConfigIndicationV1_3 == nullptr) {
        mRadioConfigResponseV1_3 = nullptr;
        mRadioConfigIndicationV1_3 = nullptr;
    }

    mRadioConfigResponseV1_2 =
            V1_2::IRadioConfigResponse::castFrom(mRadioConfigResponse).withDefault(nullptr);
    mRadioConfigIndicationV1_2 =
            V1_2::IRadioConfigIndication::castFrom(mRadioConfigIndication).withDefault(nullptr);
    if (mRadioConfigResponseV1_2 == nullptr || mRadioConfigIndicationV1_2 == nullptr) {
        mRadioConfigResponseV1_2 = nullptr;
        mRadioConfigIndicationV1_2 = nullptr;
    }

    mRadioConfigResponseV1_1 =
            V1_1::IRadioConfigResponse::castFrom(mRadioConfigResponse).withDefault(nullptr);
    mRadioConfigIndicationV1_1 =
            V1_1::IRadioConfigIndication::castFrom(mRadioConfigIndication).withDefault(nullptr);
    if (mRadioConfigResponseV1_1 == nullptr || mRadioConfigIndicationV1_1 == nullptr) {
        mRadioConfigResponseV1_1 = nullptr;
        mRadioConfigIndicationV1_1 = nullptr;
    }

    return Void();
}

Return<void> RadioConfig::getSimSlotsStatus(int32_t /* serial */) {
    hidl_vec<V1_0::SimSlotStatus> slotStatus;
    RadioResponseInfo info;
    mRadioConfigResponse->getSimSlotsStatusResponse(info, slotStatus);
    return Void();
}

Return<void> RadioConfig::setSimSlotsMapping(int32_t /* serial */,
                                             const hidl_vec<uint32_t>& /* slotMap */) {
    RadioResponseInfo info;
    mRadioConfigResponse->setSimSlotsMappingResponse(info);
    return Void();
}

// Methods from ::android::hardware::radio::config::V1_1::IRadioConfig follow.
Return<void> RadioConfig::getPhoneCapability(int32_t /* serial */) {
    V1_1::PhoneCapability phoneCapability;
    RadioResponseInfo info;
    mRadioConfigResponseV1_1->getPhoneCapabilityResponse(info, phoneCapability);
    return Void();
}

Return<void> RadioConfig::setPreferredDataModem(int32_t /* serial */, uint8_t /* modemId */) {
    RadioResponseInfo info;
    mRadioConfigResponseV1_1->setPreferredDataModemResponse(info);
    return Void();
}

Return<void> RadioConfig::setModemsConfig(int32_t /* serial */,
                                          const V1_1::ModemsConfig& /* modemsConfig */) {
    RadioResponseInfo info;
    mRadioConfigResponseV1_1->setModemsConfigResponse(info);
    return Void();
}

Return<void> RadioConfig::getModemsConfig(int32_t /* serial */) {
    V1_1::ModemsConfig modemsConfig;
    RadioResponseInfo info;
    mRadioConfigResponseV1_1->getModemsConfigResponse(info, modemsConfig);
    return Void();
}

// Methods from ::android::hardware::radio::config::V1_3::IRadioConfig follow.
Return<void> RadioConfig::getPhoneCapability_1_3(int32_t /* serial */) {
    V1_3::PhoneCapability phoneCapability;
    RadioResponseInfo info;
    mRadioConfigResponseV1_3->getPhoneCapabilityResponse_1_3(info, phoneCapability);
    return Void();
}

}  // namespace implementation
}  // namespace V1_3
}  // namespace config
}  // namespace radio
}  // namespace hardware
}  // namespace android
