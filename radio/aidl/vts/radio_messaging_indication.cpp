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

#include "radio_messaging_utils.h"

RadioMessagingIndication::RadioMessagingIndication(RadioServiceTest& parent)
    : parent_messaging(parent) {}

ndk::ScopedAStatus RadioMessagingIndication::cdmaNewSms(RadioIndicationType /*type*/,
                                                        const CdmaSmsMessage& /*msg*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioMessagingIndication::cdmaRuimSmsStorageFull(RadioIndicationType /*type*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioMessagingIndication::newBroadcastSms(RadioIndicationType /*type*/,
                                                             const std::vector<uint8_t>& /*data*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioMessagingIndication::newSms(RadioIndicationType /*type*/,
                                                    const std::vector<uint8_t>& /*pdu*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioMessagingIndication::newSmsOnSim(RadioIndicationType /*type*/,
                                                         int32_t /*recordNumber*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioMessagingIndication::newSmsStatusReport(
        RadioIndicationType /*type*/, const std::vector<uint8_t>& /*pdu*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioMessagingIndication::simSmsStorageFull(RadioIndicationType /*type*/) {
    return ndk::ScopedAStatus::ok();
}
