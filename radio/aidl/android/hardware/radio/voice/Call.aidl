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

import android.hardware.radio.voice.AudioQuality;
import android.hardware.radio.voice.UusInfo;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable Call {
    const int PRESENTATION_ALLOWED = 0;
    const int PRESENTATION_RESTRICTED = 1;
    const int PRESENTATION_UNKNOWN = 2;
    const int PRESENTATION_PAYPHONE = 3;

    const int STATE_ACTIVE = 0;
    const int STATE_HOLDING = 1;
    /**
     * MO call only
     */
    const int STATE_DIALING = 2;
    /**
     * MO call only
     */
    const int STATE_ALERTING = 3;
    /**
     * MT call only
     */
    const int STATE_INCOMING = 4;
    /**
     * MT call only
     */
    const int STATE_WAITING = 5;

    /**
     * Values are STATE_
     */
    int state;
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
    /**
     * Values are PRESENTATION_
     */
    int numberPresentation;
    /**
     * Remote party name
     */
    String name;
    /**
     * Values are PRESENTATION_
     */
    int namePresentation;
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
