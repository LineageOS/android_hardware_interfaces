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
parcelable SmsWriteArgs {
    const int STATUS_REC_UNREAD = 0;
    const int STATUS_REC_READ = 1;
    const int STATUS_STO_UNSENT = 2;
    const int STATUS_STO_SENT = 3;

    /**
     * Status of message. See TS 27.005 3.1.
     * Values are STATUS_
     */
    int status;
    /**
     * PDU of message to write, as an ASCII hex string less the SMSC address, the TP-layer length
     * is strlen(pdu)/2.
     */
    String pdu;
    /**
     * SMSC address in GSM BCD format prefixed by a length byte (as expected by TS 27.005)
     * or NULL for default SMSC.
     */
    String smsc;
}
