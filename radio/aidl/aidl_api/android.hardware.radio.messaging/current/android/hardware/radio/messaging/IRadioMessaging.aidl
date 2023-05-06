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
///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package android.hardware.radio.messaging;
/* @hide */
@VintfStability
interface IRadioMessaging {
  oneway void acknowledgeIncomingGsmSmsWithPdu(in int serial, in boolean success, in String ackPdu);
  oneway void acknowledgeLastIncomingCdmaSms(in int serial, in android.hardware.radio.messaging.CdmaSmsAck smsAck);
  oneway void acknowledgeLastIncomingGsmSms(in int serial, in boolean success, in android.hardware.radio.messaging.SmsAcknowledgeFailCause cause);
  oneway void deleteSmsOnRuim(in int serial, in int index);
  oneway void deleteSmsOnSim(in int serial, in int index);
  oneway void getCdmaBroadcastConfig(in int serial);
  oneway void getGsmBroadcastConfig(in int serial);
  oneway void getSmscAddress(in int serial);
  oneway void reportSmsMemoryStatus(in int serial, in boolean available);
  oneway void responseAcknowledgement();
  oneway void sendCdmaSms(in int serial, in android.hardware.radio.messaging.CdmaSmsMessage sms);
  oneway void sendCdmaSmsExpectMore(in int serial, in android.hardware.radio.messaging.CdmaSmsMessage sms);
  oneway void sendImsSms(in int serial, in android.hardware.radio.messaging.ImsSmsMessage message);
  oneway void sendSms(in int serial, in android.hardware.radio.messaging.GsmSmsMessage message);
  oneway void sendSmsExpectMore(in int serial, in android.hardware.radio.messaging.GsmSmsMessage message);
  oneway void setCdmaBroadcastActivation(in int serial, in boolean activate);
  oneway void setCdmaBroadcastConfig(in int serial, in android.hardware.radio.messaging.CdmaBroadcastSmsConfigInfo[] configInfo);
  oneway void setGsmBroadcastActivation(in int serial, in boolean activate);
  oneway void setGsmBroadcastConfig(in int serial, in android.hardware.radio.messaging.GsmBroadcastSmsConfigInfo[] configInfo);
  oneway void setResponseFunctions(in android.hardware.radio.messaging.IRadioMessagingResponse radioMessagingResponse, in android.hardware.radio.messaging.IRadioMessagingIndication radioMessagingIndication);
  oneway void setSmscAddress(in int serial, in String smsc);
  oneway void writeSmsToRuim(in int serial, in android.hardware.radio.messaging.CdmaSmsWriteArgs cdmaSms);
  oneway void writeSmsToSim(in int serial, in android.hardware.radio.messaging.SmsWriteArgs smsWriteArgs);
}
