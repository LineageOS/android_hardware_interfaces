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

import android.hardware.radio.AccessNetwork;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable ImsCall {
    @Backing(type="int")
    enum CallType {
        NORMAL,
        EMERGENCY,
    }

    @Backing(type="int")
    enum CallState {
        ACTIVE,
        HOLDING,
        DIALING, /* Outgoing only */
        ALERTING, /* Outgoing only */
        INCOMING, /* Incoming only */
        WAITING, /* Incoming only */
        DISCONNECTING,
        DISCONNECTED,
    }

    @Backing(type="int")
    enum Direction {
        INCOMING,
        OUTGOING,
    }

    /** Call index */
    int index;

    /** The type of the call */
    CallType callType;

    /** The access network where the call is in progress */
    AccessNetwork accessNetwork;

    /** The state of the call */
    CallState callState;

    /** The direction of the call */
    Direction direction;

    /** True if the call is put on HOLD by the other party */
    boolean isHeldByRemote;
}
