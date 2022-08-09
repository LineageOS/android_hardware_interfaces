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

#include "structs.h"

#include "commonStructs.h"

#include "collections.h"

#include <android-base/logging.h>

namespace android::hardware::radio::compat {

namespace aidl = ::aidl::android::hardware::radio::sim;

V1_0::IccIo toHidl(const aidl::IccIo& icc) {
    return {
            .command = icc.command,
            .fileId = icc.fileId,
            .path = icc.path,
            .p1 = icc.p1,
            .p2 = icc.p2,
            .p3 = icc.p3,
            .data = icc.data,
            .pin2 = icc.pin2,
            .aid = icc.aid,
    };
}

V1_0::SimApdu toHidl(const aidl::SimApdu& apdu) {
    return {
            .sessionId = apdu.sessionId,
            .cla = apdu.cla,
            .instruction = apdu.instruction,
            .p1 = apdu.p1,
            .p2 = apdu.p2,
            .p3 = apdu.p3,
            .data = apdu.data,
    };
}

aidl::Carrier toAidl(const V1_0::Carrier& carrier) {
    return {
            .mcc = carrier.mcc,
            .mnc = carrier.mnc,
            .matchType = static_cast<int32_t>(carrier.matchType),
            .matchData = carrier.matchData,
    };
}

V1_0::Carrier toHidl(const aidl::Carrier& carrier) {
    return {
            .mcc = carrier.mcc,
            .mnc = carrier.mnc,
            .matchType = static_cast<V1_0::CarrierMatchType>(carrier.matchType),
            .matchData = carrier.matchData,
    };
}

aidl::CarrierRestrictions toAidl(const V1_0::CarrierRestrictions& cr) {
    return {
            .allowedCarriers = toAidl(cr.allowedCarriers),
            .excludedCarriers = toAidl(cr.excludedCarriers),
            .allowedCarriersPrioritized = true,
    };
}

aidl::CarrierRestrictions toAidl(const V1_4::CarrierRestrictionsWithPriority& cr) {
    return {
            .allowedCarriers = toAidl(cr.allowedCarriers),
            .excludedCarriers = toAidl(cr.excludedCarriers),
            .allowedCarriersPrioritized = cr.allowedCarriersPrioritized,
    };
}

V1_4::CarrierRestrictionsWithPriority toHidl(const aidl::CarrierRestrictions& cr) {
    return {
            .allowedCarriers = toHidl(cr.allowedCarriers),
            .excludedCarriers = toHidl(cr.excludedCarriers),
            .allowedCarriersPrioritized = cr.allowedCarriersPrioritized,
    };
}

V1_1::ImsiEncryptionInfo toHidl(const aidl::ImsiEncryptionInfo& info) {
    return {
            .mcc = info.mcc,
            .mnc = info.mnc,
            .carrierKey = info.carrierKey,
            .keyIdentifier = info.keyIdentifier,
            .expirationTime = info.expirationTime,
    };
}

V1_6::ImsiEncryptionInfo toHidl_1_6(const aidl::ImsiEncryptionInfo& info) {
    return {
            .base = toHidl(info),
            .keyType = static_cast<V1_6::PublicKeyType>(info.keyType),
    };
}

V1_0::SelectUiccSub toHidl(const aidl::SelectUiccSub& sub) {
    return {
            .slot = sub.slot,
            .appIndex = sub.appIndex,
            .subType = {},
            .actStatus = {},
    };
}

aidl::PhonebookRecordInfo toAidl(const V1_6::PhonebookRecordInfo& info) {
    return {
            .recordId = static_cast<int32_t>(info.recordId),
            .name = info.name,
            .number = info.number,
            .emails = toAidl(info.emails),
            .additionalNumbers = toAidl(info.additionalNumbers),
    };
}

V1_6::PhonebookRecordInfo toHidl(const aidl::PhonebookRecordInfo& info) {
    return {
            .recordId = static_cast<uint32_t>(info.recordId),
            .name = info.name,
            .number = info.number,
            .emails = toHidl(info.emails),
            .additionalNumbers = toHidl(info.additionalNumbers),
    };
}

aidl::SimRefreshResult toAidl(const V1_0::SimRefreshResult& res) {
    return {
            .type = static_cast<int32_t>(res.type),
            .efId = res.efId,
            .aid = res.aid,
    };
}

aidl::CardStatus toAidl(const V1_0::CardStatus& status) {
    return toAidl(V1_2::CardStatus{status, 0, "", ""});
}

aidl::CardStatus toAidl(const V1_2::CardStatus& status) {
    return toAidl(V1_4::CardStatus{status, ""});
}

aidl::CardStatus toAidl(const V1_4::CardStatus& status) {
    auto aidlStatus = toAidl(V1_5::CardStatus{status, {}});
    aidlStatus.applications = toAidl(status.base.base.applications);
    return aidlStatus;
}

aidl::CardStatus toAidl(const V1_5::CardStatus& status) {
    return {
            .cardState = static_cast<int32_t>(status.base.base.base.cardState),
            .universalPinState = aidl::PinState(status.base.base.base.universalPinState),
            .gsmUmtsSubscriptionAppIndex = status.base.base.base.gsmUmtsSubscriptionAppIndex,
            .cdmaSubscriptionAppIndex = status.base.base.base.cdmaSubscriptionAppIndex,
            .imsSubscriptionAppIndex = status.base.base.base.imsSubscriptionAppIndex,
            .applications = toAidl(status.applications),
            .atr = status.base.base.atr,
            .iccid = status.base.base.iccid,
            .eid = status.base.eid,
            .slotMap = {static_cast<int32_t>(status.base.base.physicalSlotId), 0},
    };
}

aidl::AppStatus toAidl(const V1_0::AppStatus& status) {
    return toAidl({status, V1_5::PersoSubstate(status.persoSubstate)});
}

aidl::AppStatus toAidl(const V1_5::AppStatus& status) {
    return {
            .appType = static_cast<int32_t>(status.base.appType),
            .appState = static_cast<int32_t>(status.base.appState),
            .persoSubstate = aidl::PersoSubstate(status.persoSubstate),
            .aidPtr = status.base.aidPtr,
            .appLabelPtr = status.base.appLabelPtr,
            .pin1Replaced = (status.base.pin1Replaced != 0),
            .pin1 = aidl::PinState(status.base.pin1),
            .pin2 = aidl::PinState(status.base.pin2),
    };
}

aidl::PhonebookCapacity toAidl(const V1_6::PhonebookCapacity& c) {
    return {
            .maxAdnRecords = c.maxAdnRecords,
            .usedAdnRecords = c.usedAdnRecords,
            .maxEmailRecords = c.maxEmailRecords,
            .usedEmailRecords = c.usedEmailRecords,
            .maxAdditionalNumberRecords = c.maxAdditionalNumberRecords,
            .usedAdditionalNumberRecords = c.usedAdditionalNumberRecords,
            .maxNameLen = c.maxNameLen,
            .maxNumberLen = c.maxNumberLen,
            .maxEmailLen = c.maxEmailLen,
            .maxAdditionalNumberLen = c.maxAdditionalNumberLen,
    };
}

aidl::IccIoResult toAidl(const V1_0::IccIoResult& iir) {
    return {
            .sw1 = iir.sw1,
            .sw2 = iir.sw2,
            .simResponse = iir.simResponse,
    };
}

}  // namespace android::hardware::radio::compat
