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

package android.hardware.radio.messaging;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable GsmSmsMessage {
    /**
     * SMSC address in GSM BCD format prefixed by a length byte (as expected by TS 27.005)
     * or empty string for default SMSC
     */
    String smscPdu;
    /**
     * SMS in PDU format as an ASCII hex string less the SMSC address.
     * TP-Layer-Length is be "strlen(pdu)/2
     * TP - MessageRef field of pdu must not be modified by modem
     */
    String pdu;
}
