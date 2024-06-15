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

package android.hardware.power;

import android.hardware.power.SessionHint;
import android.hardware.power.SessionMode;
import android.hardware.power.WorkDurationFixedV1;

/**
 * Data sent through the FMQ must follow this structure. It's important to note
 * that such data may come from the app itself, so the HAL must validate all
 * data received through this interface, and reject any calls not guaranteed to be
 * valid. Each of the types defined in the inner union maps to an equivalent call
 * on IPowerHintSession, and is merely being used to expedite the use of that API
 * in cases where it is safe to bypass the HintManagerService.
 */
@FixedSize
@VintfStability
parcelable ChannelMessage {
    /**
     * The ID of the specific session sending the hint, used to enable a single
     * channel to be multiplexed across all sessions in a single process.
     */
    int sessionID;

    /**
     * Timestamp in nanoseconds based on CLOCK_MONOTONIC when the message was sent,
     * used to ensure all messages can be processed in a coherent order.
     */
    long timeStampNanos;

    /**
     * A union defining the different messages that can be passed through the
     * channel. Each type corresponds to a different call in IPowerHintSession.
     */
    ChannelMessageContents data;

    @FixedSize
    @VintfStability
    union ChannelMessageContents {
        /**
         * Reserves the maximum fixed size for the ChannelMessage.
         */
        long[16] reserved = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

        /**
         * Setting this field will update the session’s target duration, equivalent
         * to calling updateTargetWorkDuration(targetDuration).
         */
        long targetDuration;

        /**
         * Setting this field will send a hint to the session, equivalent to
         * calling sendHint(hint).
         */
        SessionHint hint;

        /**
         * Setting this field will send a hint to the session, equivalent to
         * calling setMode(mode.modeInt, mode.enabled).
         */
        SessionModeSetter mode;

        /**
         * Setting this field will update the session’s actual duration, equivalent
         * to calling reportActualWorkDuration([workDuration]). Only one duration
         * can be passed at a time; this API expects durations to be reported
         * immediately each frame, since the overhead of this call is much lower.
         */
        WorkDurationFixedV1 workDuration;

        /**
         * This structure is used to fit both the mode and the state within one
         * entry in the union.
         */
        @FixedSize
        @VintfStability
        parcelable SessionModeSetter {
            SessionMode modeInt;
            boolean enabled;
        }
    }
}
