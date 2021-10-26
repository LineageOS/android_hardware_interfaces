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

#include <libradiocompat/RadioIndication.h>

#include "commonStructs.h"
#include "debug.h"
#include "structs.h"

#define RADIO_MODULE "MessagingIndication"

namespace android::hardware::radio::compat {

namespace aidl = ::aidl::android::hardware::radio::messaging;

void RadioIndication::setResponseFunction(std::shared_ptr<aidl::IRadioMessagingIndication> rmiCb) {
    CHECK(rmiCb);
    mMessagingCb = rmiCb;
}

Return<void> RadioIndication::cdmaNewSms(V1_0::RadioIndicationType type,
                                         const V1_0::CdmaSmsMessage& msg) {
    LOG_CALL << type;
    CHECK_CB(mMessagingCb);
    mMessagingCb->cdmaNewSms(toAidl(type), toAidl(msg));
    return {};
}

Return<void> RadioIndication::cdmaRuimSmsStorageFull(V1_0::RadioIndicationType type) {
    LOG_CALL << type;
    CHECK_CB(mMessagingCb);
    mMessagingCb->cdmaRuimSmsStorageFull(toAidl(type));
    return {};
}

Return<void> RadioIndication::newBroadcastSms(V1_0::RadioIndicationType type,
                                              const hidl_vec<uint8_t>& data) {
    LOG_CALL << type;
    CHECK_CB(mMessagingCb);
    mMessagingCb->newBroadcastSms(toAidl(type), data);
    return {};
}

Return<void> RadioIndication::newSms(V1_0::RadioIndicationType type, const hidl_vec<uint8_t>& pdu) {
    LOG_CALL << type;
    CHECK_CB(mMessagingCb);
    mMessagingCb->newSms(toAidl(type), pdu);
    return {};
}

Return<void> RadioIndication::newSmsOnSim(V1_0::RadioIndicationType type, int32_t recordNumber) {
    LOG_CALL << type;
    CHECK_CB(mMessagingCb);
    mMessagingCb->newSmsOnSim(toAidl(type), recordNumber);
    return {};
}

Return<void> RadioIndication::newSmsStatusReport(V1_0::RadioIndicationType type,
                                                 const hidl_vec<uint8_t>& pdu) {
    LOG_CALL << type;
    CHECK_CB(mMessagingCb);
    mMessagingCb->newSmsStatusReport(toAidl(type), pdu);
    return {};
}

Return<void> RadioIndication::onUssd(V1_0::RadioIndicationType type, V1_0::UssdModeType modeType,
                                     const hidl_string& msg) {
    LOG_CALL << type;
    CHECK_CB(mMessagingCb);
    mMessagingCb->onUssd(toAidl(type), aidl::UssdModeType(modeType), msg);
    return {};
}

Return<void> RadioIndication::simSmsStorageFull(V1_0::RadioIndicationType type) {
    LOG_CALL << type;
    CHECK_CB(mMessagingCb);
    mMessagingCb->simSmsStorageFull(toAidl(type));
    return {};
}

}  // namespace android::hardware::radio::compat
