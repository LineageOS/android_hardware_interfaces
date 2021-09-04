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

package android.hardware.radio;

import android.hardware.radio.AudioQuality;
import android.hardware.radio.CallPresentation;
import android.hardware.radio.CallState;
import android.hardware.radio.UusInfo;

@VintfStability
parcelable Call {
    CallState state;
    /**
     * Connection index for use with, eg, AT+CHLD
     */
    int index;
    /**
     * Type of address, eg 145 = intl
     */
    int toa;
    /**
     * true if is mpty call
     */
    boolean isMpty;
    /**
     * true if call is mobile terminated
     */
    boolean isMT;
    /**
     * ALS line indicator if availale (0 = line 1)
     */
    byte als;
    /**
     * true if this is a voice call
     */
    boolean isVoice;
    /**
     * true if CDMA voice privacy mode is active
     */
    boolean isVoicePrivacy;
    /**
     * Remote party nummber
     */
    String number;
    CallPresentation numberPresentation;
    /**
     * Remote party name
     */
    String name;
    CallPresentation namePresentation;
    /**
     * Vector of User-User Signaling Information
     */
    UusInfo[] uusInfo;
    AudioQuality audioQuality;
    /**
     * Forwarded number. It can set only one forwarded number based on 3GPP rule of the CS.
     * Reference: 3GPP TS 24.008 section 10.5.4.21b
     */
    String forwardedNumber;
}
