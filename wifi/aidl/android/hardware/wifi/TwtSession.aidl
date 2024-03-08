/*
 * Copyright (C) 2023 The Android Open Source Project
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

package android.hardware.wifi;

/**
 * Target Wake Time (TWT) Session
 */
@VintfStability
parcelable TwtSession {
    @VintfStability
    @Backing(type="byte")
    enum TwtNegotiationType {
        INDIVIDUAL = 0,
        BROADCAST = 1,
    }

    /**
     * An unique identifier for the session.
     */
    int sessionId;

    /**
     * MLO Link id in case of MLO connection. Otherwise -1.
     */
    int mloLinkId;

    /**
     * TWT service period in microseconds.
     */
    int wakeDurationMicros;

    /**
     * Time interval in microseconds between two successive TWT service periods.
     */
    int wakeIntervalMicros;

    /**
     * TWT negotiation type.
     */
    TwtNegotiationType negotiationType;

    /**
     * Whether the TWT session is trigger enabled or non-trigger enabled.
     */
    boolean isTriggerEnabled;

    /**
     * Whether the TWT session is announced or unannounced.
     */
    boolean isAnnounced;

    /**
     * Whether the TWT session is implicit or explicit.
     */
    boolean isImplicit;

    /**
     * Whether the TWT session is protected or not.
     */
    boolean isProtected;

    /**
     * Whether the TWT session can be updated.
     */
    boolean isUpdatable;

    /**
     * Whether the TWT session can be suspended and then resumed.
     */
    boolean isSuspendable;

    /**
     * Whether AP (TWT responder) intends to go to doze state outside of TWT Service Periods.
     *
     * Refer IEEE 802.11 spec, Section 10.47.7 (TWT Sleep Setup).
     */
    boolean isResponderPmModeEnabled;
}
