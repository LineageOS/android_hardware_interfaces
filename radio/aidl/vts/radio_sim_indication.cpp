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

#include "radio_sim_utils.h"

RadioSimIndication::RadioSimIndication(RadioServiceTest& parent) : parent_sim(parent) {}

ndk::ScopedAStatus RadioSimIndication::carrierInfoForImsiEncryption(RadioIndicationType /*info*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimIndication::cdmaSubscriptionSourceChanged(
        RadioIndicationType /*type*/, CdmaSubscriptionSource /*cdmaSource*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimIndication::simPhonebookChanged(RadioIndicationType /*type*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimIndication::simPhonebookRecordsReceived(
        RadioIndicationType /*type*/, PbReceivedStatus /*status*/,
        const std::vector<PhonebookRecordInfo>& /*records*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimIndication::simRefresh(RadioIndicationType /*type*/,
                                                  const SimRefreshResult& /*refreshResult*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimIndication::simStatusChanged(RadioIndicationType /*type*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimIndication::stkEventNotify(RadioIndicationType /*type*/,
                                                      const std::string& /*cmd*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimIndication::stkProactiveCommand(RadioIndicationType /*type*/,
                                                           const std::string& /*cmd*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimIndication::stkSessionEnd(RadioIndicationType /*type*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimIndication::subscriptionStatusChanged(RadioIndicationType /*type*/,
                                                                 bool /*activate*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSimIndication::uiccApplicationsEnablementChanged(
        RadioIndicationType /*type*/, bool /*enabled*/) {
    return ndk::ScopedAStatus::ok();
}
