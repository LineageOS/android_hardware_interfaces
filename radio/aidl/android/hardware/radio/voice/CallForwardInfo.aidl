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

package android.hardware.radio.voice;

/**
 * See also com.android.internal.telephony.gsm.CallForwardInfo
 * @hide
 */
@VintfStability
@JavaDerive(toString=true)
parcelable CallForwardInfo {
    const int STATUS_DISABLE = 0;
    const int STATUS_ENABLE = 1;
    const int STATUS_INTERROGATE = 2;
    const int STATUS_REGISTRATION = 3;
    const int STATUS_ERASURE = 4;

    /**
     * For queryCallForwardStatus() status is STATUS_DISABLE (Not used by vendor code currently)
     * For setCallForward() status must be STATUS_DISABLE, STATUS_ENABLE, STATUS_INTERROGATE,
     * STATUS_REGISTRATION, STATUS_ERASURE
     * Values are STATUS_
     */
    int status;
    /**
     * From TS 27.007 7.11 "reason"
     */
    int reason;
    /**
     * From TS 27.007 +CCFC/+CLCK "class". See table for Android mapping from MMI service code.
     * 0 means user doesn't input class.
     */
    int serviceClass;
    /**
     * From TS 27.007 7.11 "type"
     */
    int toa;
    /**
     * From TS 27.007 7.11 "number"
     */
    String number;
    int timeSeconds;
}
