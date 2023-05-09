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
parcelable SendSmsResult {
    /**
     * TP-Message-Reference for GSM, and BearerData MessageId for CDMA.
     * See 3GPP2 C.S0015-B, v2.0, table 4.5-1
     */
    int messageRef;
    /**
     * Ack PDU or empty string if n/a
     */
    String ackPDU;
    /**
     * See 3GPP 27.005, 3.2.5 for GSM/UMTS, 3GPP2 N.S0005 (IS-41C) Table 171 for CDMA.
     * -1 if unknown or not applicable.
     */
    int errorCode;
}
