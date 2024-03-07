/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.radio.ims;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable SrvccCall {
    @VintfStability
    @Backing(type="int")
    enum CallType {
        NORMAL,
        EMERGENCY,
    }

    @VintfStability
    @Backing(type="int")
    enum CallSubState {
        NONE,
        /** Pre-alerting state. Applicable for MT calls only */
        PREALERTING,
    }

    @VintfStability
    @Backing(type="int")
    enum ToneType {
        NONE,
        LOCAL,
        NETWORK,
    }

    /** Connection index */
    int index;

    /** The type of the call */
    CallType callType;

    /** Values are android.hardware.radio.voice.Call.STATE_* constants */
    int callState;

    /** The substate of the call */
    CallSubState callSubstate;

    /** The type of the ringback tone */
    ToneType ringbackToneType;

    /** true if is mpty call */
    boolean isMpty;

    /** true if call is mobile terminated */
    boolean isMT;

    /** Remote party nummber */
    String number;

    /** Values are android.hardware.radio.voice.Call.PRESENTATION_* constants */
    int numPresentation;

    /** Remote party name */
    String name;

    /** Values are android.hardware.radio.voice.Call.PRESENTATION_* constants */
    int namePresentation;
}
