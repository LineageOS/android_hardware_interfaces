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

#include "collections.h"

#include <aidl/android/hardware/radio/messaging/CdmaSmsAddress.h>
#include <android-base/logging.h>

namespace android::hardware::radio::compat {

namespace aidl = ::aidl::android::hardware::radio::messaging;

V1_0::CdmaSmsAck toHidl(const aidl::CdmaSmsAck& smsAck) {
    return {
            .errorClass = (smsAck.errorClass ? V1_0::CdmaSmsErrorClass::ERROR
                                             : V1_0::CdmaSmsErrorClass::NO_ERROR),
            .smsCauseCode = smsAck.smsCauseCode,
    };
}

static aidl::CdmaSmsAddress toAidl(const V1_0::CdmaSmsAddress& addr) {
    return {
            .digitMode = static_cast<int32_t>(addr.digitMode),
            .isNumberModeDataNetwork = addr.numberMode == V1_0::CdmaSmsNumberMode::DATA_NETWORK,
            .numberType = static_cast<int32_t>(addr.numberType),
            .numberPlan = static_cast<int32_t>(addr.numberPlan),
            .digits = addr.digits,
    };
}

static V1_0::CdmaSmsAddress toHidl(const aidl::CdmaSmsAddress& addr) {
    return {
            .digitMode = static_cast<V1_0::CdmaSmsDigitMode>(addr.digitMode),
            .numberMode = addr.isNumberModeDataNetwork ? V1_0::CdmaSmsNumberMode::DATA_NETWORK
                                                       : V1_0::CdmaSmsNumberMode::NOT_DATA_NETWORK,
            .numberType = static_cast<V1_0::CdmaSmsNumberType>(addr.numberType),
            .numberPlan = static_cast<V1_0::CdmaSmsNumberPlan>(addr.numberPlan),
            .digits = addr.digits,
    };
}

static aidl::CdmaSmsSubaddress toAidl(const V1_0::CdmaSmsSubaddress& addr) {
    return {
            .subaddressType = static_cast<int32_t>(addr.subaddressType),
            .odd = addr.odd,
            .digits = addr.digits,
    };
}

static V1_0::CdmaSmsSubaddress toHidl(const aidl::CdmaSmsSubaddress& addr) {
    return {
            .subaddressType = static_cast<V1_0::CdmaSmsSubaddressType>(addr.subaddressType),
            .odd = addr.odd,
            .digits = addr.digits,
    };
}

::aidl::android::hardware::radio::messaging::CdmaSmsMessage toAidl(const V1_0::CdmaSmsMessage& m) {
    return {
            .teleserviceId = m.teleserviceId,
            .isServicePresent = m.isServicePresent,
            .serviceCategory = m.serviceCategory,
            .address = toAidl(m.address),
            .subAddress = toAidl(m.subAddress),
            .bearerData = m.bearerData,
    };
}

V1_0::CdmaSmsMessage toHidl(const aidl::CdmaSmsMessage& msg) {
    return {
            .teleserviceId = msg.teleserviceId,
            .isServicePresent = msg.isServicePresent,
            .serviceCategory = msg.serviceCategory,
            .address = toHidl(msg.address),
            .subAddress = toHidl(msg.subAddress),
            .bearerData = msg.bearerData,
    };
}

V1_0::ImsSmsMessage toHidl(const aidl::ImsSmsMessage& msg) {
    return {
            .tech = static_cast<V1_0::RadioTechnologyFamily>(msg.tech),
            .retry = msg.retry,
            .messageRef = msg.messageRef,
            .cdmaMessage = toHidl(msg.cdmaMessage),
            .gsmMessage = toHidl(msg.gsmMessage),
    };
}

V1_0::GsmSmsMessage toHidl(const aidl::GsmSmsMessage& msg) {
    return {
            .smscPdu = msg.smscPdu,
            .pdu = msg.pdu,
    };
}

aidl::CdmaBroadcastSmsConfigInfo toAidl(const V1_0::CdmaBroadcastSmsConfigInfo& info) {
    return {
            .serviceCategory = info.serviceCategory,
            .language = info.language,
            .selected = info.selected,
    };
}

V1_0::CdmaBroadcastSmsConfigInfo toHidl(const aidl::CdmaBroadcastSmsConfigInfo& info) {
    return {
            .serviceCategory = info.serviceCategory,
            .language = info.language,
            .selected = info.selected,
    };
}

aidl::GsmBroadcastSmsConfigInfo toAidl(const V1_0::GsmBroadcastSmsConfigInfo& info) {
    return {
            .fromServiceId = info.fromServiceId,
            .toServiceId = info.toServiceId,
            .fromCodeScheme = info.fromCodeScheme,
            .toCodeScheme = info.toCodeScheme,
            .selected = info.selected,
    };
}

V1_0::GsmBroadcastSmsConfigInfo toHidl(const aidl::GsmBroadcastSmsConfigInfo& info) {
    return {
            .fromServiceId = info.fromServiceId,
            .toServiceId = info.toServiceId,
            .fromCodeScheme = info.fromCodeScheme,
            .toCodeScheme = info.toCodeScheme,
            .selected = info.selected,
    };
}

V1_0::CdmaSmsWriteArgs toHidl(const aidl::CdmaSmsWriteArgs& args) {
    return {
            .status = static_cast<V1_0::CdmaSmsWriteArgsStatus>(args.status),
            .message = toHidl(args.message),
    };
}

V1_0::SmsWriteArgs toHidl(const aidl::SmsWriteArgs& args) {
    return {
            .status = static_cast<V1_0::SmsWriteArgsStatus>(args.status),
            .pdu = args.pdu,
            .smsc = args.smsc,
    };
}

::aidl::android::hardware::radio::messaging::SendSmsResult toAidl(
        const V1_0::SendSmsResult& result) {
    return {
            .messageRef = result.messageRef,
            .ackPDU = result.ackPDU,
            .errorCode = result.errorCode,
    };
}

}  // namespace android::hardware::radio::compat
