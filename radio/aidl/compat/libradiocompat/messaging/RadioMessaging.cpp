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

#include <libradiocompat/RadioMessaging.h>

#include "debug.h"
#include "structs.h"

#include "collections.h"

#define RADIO_MODULE "Messaging"

namespace android::hardware::radio::compat {

using ::ndk::ScopedAStatus;
namespace aidl = ::aidl::android::hardware::radio::messaging;
constexpr auto ok = &ScopedAStatus::ok;

std::shared_ptr<aidl::IRadioMessagingResponse> RadioMessaging::respond() {
    return mCallbackManager->response().messagingCb();
}

ScopedAStatus RadioMessaging::acknowledgeIncomingGsmSmsWithPdu(  //
        int32_t serial, bool success, const std::string& ackPdu) {
    LOG_CALL << serial << ' ' << success << ' ' << ackPdu;
    mHal1_5->acknowledgeIncomingGsmSmsWithPdu(serial, success, ackPdu);
    return ok();
}

ScopedAStatus RadioMessaging::acknowledgeLastIncomingCdmaSms(  //
        int32_t serial, const aidl::CdmaSmsAck& smsAck) {
    LOG_CALL << serial;
    mHal1_5->acknowledgeLastIncomingCdmaSms(serial, toHidl(smsAck));
    return ok();
}

ScopedAStatus RadioMessaging::acknowledgeLastIncomingGsmSms(  //
        int32_t serial, bool success, aidl::SmsAcknowledgeFailCause cause) {
    LOG_CALL << serial << ' ' << success;
    mHal1_5->acknowledgeLastIncomingGsmSms(serial, success, V1_0::SmsAcknowledgeFailCause(cause));
    return ok();
}

ScopedAStatus RadioMessaging::deleteSmsOnRuim(int32_t serial, int32_t index) {
    LOG_CALL << serial << ' ' << index;
    mHal1_5->deleteSmsOnRuim(serial, index);
    return ok();
}

ScopedAStatus RadioMessaging::deleteSmsOnSim(int32_t serial, int32_t index) {
    LOG_CALL << serial << ' ' << index;
    mHal1_5->deleteSmsOnSim(serial, index);
    return ok();
}

ScopedAStatus RadioMessaging::getCdmaBroadcastConfig(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->getCdmaBroadcastConfig(serial);
    return ok();
}

ScopedAStatus RadioMessaging::getGsmBroadcastConfig(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->getGsmBroadcastConfig(serial);
    return ok();
}

ScopedAStatus RadioMessaging::getSmscAddress(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->getSmscAddress(serial);
    return ok();
}

ScopedAStatus RadioMessaging::reportSmsMemoryStatus(int32_t serial, bool available) {
    LOG_CALL << serial << ' ' << available;
    mHal1_5->reportSmsMemoryStatus(serial, available);
    return ok();
}

ScopedAStatus RadioMessaging::responseAcknowledgement() {
    LOG_CALL;
    mHal1_5->responseAcknowledgement();
    return ok();
}

ScopedAStatus RadioMessaging::sendCdmaSms(int32_t serial, const aidl::CdmaSmsMessage& sms) {
    LOG_CALL << serial;
    if (mHal1_6) {
        mHal1_6->sendCdmaSms_1_6(serial, toHidl(sms));
    } else {
        mHal1_5->sendCdmaSms(serial, toHidl(sms));
    }
    return ok();
}

ScopedAStatus RadioMessaging::sendCdmaSmsExpectMore(int32_t serial, const aidl::CdmaSmsMessage& m) {
    LOG_CALL << serial;
    if (mHal1_6) {
        mHal1_6->sendCdmaSmsExpectMore_1_6(serial, toHidl(m));
    } else {
        mHal1_5->sendCdmaSmsExpectMore(serial, toHidl(m));
    }
    return ok();
}

ScopedAStatus RadioMessaging::sendImsSms(int32_t serial, const aidl::ImsSmsMessage& message) {
    LOG_CALL << serial;
    mHal1_5->sendImsSms(serial, toHidl(message));
    return ok();
}

ScopedAStatus RadioMessaging::sendSms(int32_t serial, const aidl::GsmSmsMessage& message) {
    LOG_CALL << serial;
    if (mHal1_6) {
        mHal1_6->sendSms_1_6(serial, toHidl(message));
    } else {
        mHal1_5->sendSms(serial, toHidl(message));
    }
    return ok();
}

ScopedAStatus RadioMessaging::sendSmsExpectMore(int32_t serial, const aidl::GsmSmsMessage& msg) {
    LOG_CALL << serial;
    if (mHal1_6) {
        mHal1_6->sendSmsExpectMore_1_6(serial, toHidl(msg));
    } else {
        mHal1_5->sendSMSExpectMore(serial, toHidl(msg));
    }
    return ok();
}

ScopedAStatus RadioMessaging::setCdmaBroadcastActivation(int32_t serial, bool activate) {
    LOG_CALL << serial << ' ' << activate;
    mHal1_5->setCdmaBroadcastActivation(serial, activate);
    return ok();
}

ScopedAStatus RadioMessaging::setCdmaBroadcastConfig(
        int32_t serial, const std::vector<aidl::CdmaBroadcastSmsConfigInfo>& cfgInfo) {
    LOG_CALL << serial;
    mHal1_5->setCdmaBroadcastConfig(serial, toHidl(cfgInfo));
    return ok();
}

ScopedAStatus RadioMessaging::setGsmBroadcastActivation(int32_t serial, bool activate) {
    LOG_CALL << serial << ' ' << activate;
    mHal1_5->setGsmBroadcastActivation(serial, activate);
    return ok();
}

ScopedAStatus RadioMessaging::setGsmBroadcastConfig(
        int32_t serial, const std::vector<aidl::GsmBroadcastSmsConfigInfo>& configInfo) {
    LOG_CALL << serial;
    mHal1_5->setGsmBroadcastConfig(serial, toHidl(configInfo));
    return ok();
}

ScopedAStatus RadioMessaging::setResponseFunctions(
        const std::shared_ptr<aidl::IRadioMessagingResponse>& response,
        const std::shared_ptr<aidl::IRadioMessagingIndication>& indication) {
    LOG_CALL << response << ' ' << indication;
    mCallbackManager->setResponseFunctions(response, indication);
    return ok();
}

ScopedAStatus RadioMessaging::setSmscAddress(int32_t serial, const std::string& smsc) {
    LOG_CALL << serial << ' ' << smsc;
    mHal1_5->setSmscAddress(serial, smsc);
    return ok();
}

ScopedAStatus RadioMessaging::writeSmsToRuim(int32_t serial, const aidl::CdmaSmsWriteArgs& sms) {
    LOG_CALL << serial;
    mHal1_5->writeSmsToRuim(serial, toHidl(sms));
    return ok();
}

ScopedAStatus RadioMessaging::writeSmsToSim(int32_t serial, const aidl::SmsWriteArgs& smsWrArgs) {
    LOG_CALL << serial;
    mHal1_5->writeSmsToSim(serial, toHidl(smsWrArgs));
    return ok();
}

}  // namespace android::hardware::radio::compat
