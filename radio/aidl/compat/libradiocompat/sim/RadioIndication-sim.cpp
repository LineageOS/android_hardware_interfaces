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

#include "collections.h"

#define RADIO_MODULE "SimIndication"

namespace android::hardware::radio::compat {

namespace aidl = ::aidl::android::hardware::radio::sim;

void RadioIndication::setResponseFunction(std::shared_ptr<aidl::IRadioSimIndication> simCb) {
    mSimCb = simCb;
}

std::shared_ptr<aidl::IRadioSimIndication> RadioIndication::simCb() {
    return mSimCb.get();
}

Return<void> RadioIndication::carrierInfoForImsiEncryption(V1_0::RadioIndicationType type) {
    LOG_CALL << type;
    simCb()->carrierInfoForImsiEncryption(toAidl(type));
    return {};
}

Return<void> RadioIndication::cdmaSubscriptionSourceChanged(
        V1_0::RadioIndicationType type, V1_0::CdmaSubscriptionSource cdmaSource) {
    LOG_CALL << type;
    simCb()->cdmaSubscriptionSourceChanged(toAidl(type), aidl::CdmaSubscriptionSource(cdmaSource));
    return {};
}

Return<void> RadioIndication::simPhonebookChanged(V1_0::RadioIndicationType type) {
    LOG_CALL << type;
    simCb()->simPhonebookChanged(toAidl(type));
    return {};
}

Return<void> RadioIndication::simPhonebookRecordsReceived(
        V1_0::RadioIndicationType type, V1_6::PbReceivedStatus status,
        const hidl_vec<V1_6::PhonebookRecordInfo>& rec) {
    LOG_CALL << type;
    simCb()->simPhonebookRecordsReceived(toAidl(type), aidl::PbReceivedStatus(status), toAidl(rec));
    return {};
}

Return<void> RadioIndication::simRefresh(V1_0::RadioIndicationType type,
                                         const V1_0::SimRefreshResult& refreshResult) {
    LOG_CALL << type;
    simCb()->simRefresh(toAidl(type), toAidl(refreshResult));
    return {};
}

Return<void> RadioIndication::simStatusChanged(V1_0::RadioIndicationType type) {
    LOG_CALL << type;
    simCb()->simStatusChanged(toAidl(type));
    return {};
}

Return<void> RadioIndication::stkEventNotify(V1_0::RadioIndicationType type,
                                             const hidl_string& cmd) {
    LOG_CALL << type;
    simCb()->stkEventNotify(toAidl(type), cmd);
    return {};
}

Return<void> RadioIndication::stkProactiveCommand(V1_0::RadioIndicationType type,
                                                  const hidl_string& cmd) {
    LOG_CALL << type;
    simCb()->stkProactiveCommand(toAidl(type), cmd);
    return {};
}

Return<void> RadioIndication::stkSessionEnd(V1_0::RadioIndicationType type) {
    LOG_CALL << type;
    simCb()->stkSessionEnd(toAidl(type));
    return {};
}

Return<void> RadioIndication::subscriptionStatusChanged(V1_0::RadioIndicationType type,
                                                        bool activate) {
    LOG_CALL << type;
    simCb()->subscriptionStatusChanged(toAidl(type), activate);
    return {};
}

Return<void> RadioIndication::uiccApplicationsEnablementChanged(V1_0::RadioIndicationType type,
                                                                bool enabled) {
    LOG_CALL << type;
    simCb()->uiccApplicationsEnablementChanged(toAidl(type), enabled);
    return {};
}

}  // namespace android::hardware::radio::compat
