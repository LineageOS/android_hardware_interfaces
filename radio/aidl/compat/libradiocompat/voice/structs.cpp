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

namespace aidl = ::aidl::android::hardware::radio::voice;

V1_0::Dial toHidl(const aidl::Dial& info) {
    return {
            .address = info.address,
            .clir = static_cast<V1_0::Clir>(info.clir),
            .uusInfo = toHidl(info.uusInfo),
    };
}

V1_0::UusInfo toHidl(const aidl::UusInfo& info) {
    return {
            .uusType = static_cast<V1_0::UusType>(info.uusType),
            .uusDcs = static_cast<V1_0::UusDcs>(info.uusDcs),
            .uusData = info.uusData,
    };
}

aidl::CallForwardInfo toAidl(const V1_0::CallForwardInfo& info) {
    return {
            .status = static_cast<int32_t>(info.status),
            .reason = info.reason,
            .serviceClass = info.serviceClass,
            .toa = info.toa,
            .number = info.number,
            .timeSeconds = info.timeSeconds,
    };
}

V1_0::CallForwardInfo toHidl(const aidl::CallForwardInfo& info) {
    return {
            .status = static_cast<V1_0::CallForwardInfoStatus>(info.status),
            .reason = info.reason,
            .serviceClass = info.serviceClass,
            .toa = info.toa,
            .number = info.number,
            .timeSeconds = info.timeSeconds,
    };
}

aidl::CdmaSignalInfoRecord toAidl(const V1_0::CdmaSignalInfoRecord& record) {
    return {
            .isPresent = record.isPresent,
            .signalType = record.signalType,
            .alertPitch = record.alertPitch,
            .signal = record.signal,
    };
}

aidl::CdmaCallWaiting toAidl(const V1_0::CdmaCallWaiting& call) {
    return {
            .number = call.number,
            .numberPresentation = static_cast<int32_t>(call.numberPresentation),
            .name = call.name,
            .signalInfoRecord = toAidl(call.signalInfoRecord),
            .numberType = static_cast<int32_t>(call.numberType),
            .numberPlan = static_cast<int32_t>(call.numberPlan),
    };
}

aidl::CdmaInformationRecord toAidl(const V1_0::CdmaInformationRecord& record) {
    return {
            .name = static_cast<int32_t>(record.name),
            .display = toAidl(record.display),
            .number = toAidl(record.number),
            .signal = toAidl(record.signal),
            .redir = toAidl(record.redir),
            .lineCtrl = toAidl(record.lineCtrl),
            .clir = toAidl(record.clir),
            .audioCtrl = toAidl(record.audioCtrl),
    };
}

aidl::CdmaDisplayInfoRecord toAidl(const V1_0::CdmaDisplayInfoRecord& record) {
    return {
            .alphaBuf = record.alphaBuf,
    };
}

aidl::CdmaNumberInfoRecord toAidl(const V1_0::CdmaNumberInfoRecord& record) {
    return {
            .number = record.number,
            .numberType = static_cast<int8_t>(record.numberType),
            .numberPlan = static_cast<int8_t>(record.numberPlan),
            .pi = static_cast<int8_t>(record.pi),
            .si = static_cast<int8_t>(record.si),
    };
}

aidl::CdmaRedirectingNumberInfoRecord toAidl(const V1_0::CdmaRedirectingNumberInfoRecord& record) {
    return {
            .redirectingNumber = toAidl(record.redirectingNumber),
            .redirectingReason = static_cast<int32_t>(record.redirectingReason),
    };
}

aidl::CdmaLineControlInfoRecord toAidl(const V1_0::CdmaLineControlInfoRecord& record) {
    return {
            .lineCtrlPolarityIncluded = static_cast<int8_t>(record.lineCtrlPolarityIncluded),
            .lineCtrlToggle = static_cast<int8_t>(record.lineCtrlToggle),
            .lineCtrlReverse = static_cast<int8_t>(record.lineCtrlReverse),
            .lineCtrlPowerDenial = static_cast<int8_t>(record.lineCtrlPowerDenial),
    };
}

aidl::CdmaT53ClirInfoRecord toAidl(const V1_0::CdmaT53ClirInfoRecord& record) {
    return {
            .cause = static_cast<int8_t>(record.cause),
    };
}

aidl::CdmaT53AudioControlInfoRecord toAidl(const V1_0::CdmaT53AudioControlInfoRecord& record) {
    return {
            .upLink = static_cast<int8_t>(record.upLink),
            .downLink = static_cast<int8_t>(record.downLink),
    };
}

aidl::EmergencyNumber toAidl(const V1_4::EmergencyNumber& num) {
    return {
            .number = num.number,
            .mcc = num.mcc,
            .mnc = num.mnc,
            .categories = num.categories,
            .urns = toAidl(num.urns),
            .sources = num.sources,
    };
}

aidl::StkCcUnsolSsResult toAidl(const V1_0::StkCcUnsolSsResult& res) {
    return {
            .serviceType = static_cast<int32_t>(res.serviceType),
            .requestType = static_cast<int32_t>(res.requestType),
            .teleserviceType = static_cast<int32_t>(res.teleserviceType),
            .serviceClass = res.serviceClass,
            .result = toAidl(res.result),
            .ssInfo = toAidl(res.ssInfo),
            .cfData = toAidl(res.cfData),
    };
}

aidl::SsInfoData toAidl(const V1_0::SsInfoData& info) {
    return {
            .ssInfo = info.ssInfo,
    };
}

aidl::CfData toAidl(const V1_0::CfData& data) {
    return {
            .cfInfo = toAidl(data.cfInfo),
    };
}

aidl::Call toAidl(const V1_0::Call& call) {
    return toAidl(V1_2::Call{call, {}});
}

aidl::Call toAidl(const V1_2::Call& call) {
    return toAidl(V1_6::Call{call, {}});
}

aidl::Call toAidl(const V1_6::Call& call) {
    return {
            .state = static_cast<int32_t>(call.base.base.state),
            .index = call.base.base.index,
            .toa = call.base.base.toa,
            .isMpty = call.base.base.isMpty,
            .isMT = call.base.base.isMT,
            .als = static_cast<int8_t>(call.base.base.als),
            .isVoice = call.base.base.isVoice,
            .isVoicePrivacy = call.base.base.isVoicePrivacy,
            .number = call.base.base.number,
            .numberPresentation = static_cast<int32_t>(call.base.base.numberPresentation),
            .name = call.base.base.name,
            .namePresentation = static_cast<int32_t>(call.base.base.namePresentation),
            .uusInfo = toAidl(call.base.base.uusInfo),
            .audioQuality = aidl::AudioQuality(call.base.audioQuality),
            .forwardedNumber = call.forwardedNumber,
    };
}

aidl::UusInfo toAidl(const V1_0::UusInfo& info) {
    return {
            .uusType = static_cast<int32_t>(info.uusType),
            .uusDcs = static_cast<int32_t>(info.uusDcs),
            .uusData = info.uusData,
    };
}

aidl::LastCallFailCauseInfo toAidl(const V1_0::LastCallFailCauseInfo& info) {
    return {
            .causeCode = aidl::LastCallFailCause(info.causeCode),
            .vendorCause = info.vendorCause,
    };
}

}  // namespace android::hardware::radio::compat
