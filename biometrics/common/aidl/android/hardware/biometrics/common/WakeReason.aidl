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

package android.hardware.biometrics.common;

/**
 * The wake event associated with an operation, if applicable.
 *
 * The events largely shadow constants defined in PowerManager but they may deviate over time.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum WakeReason {
    /**
     * A normal operation without an explicit reason.
     */
    UNKNOWN,

    /**
     * Waking up due to power button press.
     */
    POWER_BUTTON,

    /**
     * Waking up due to a user performed gesture. This includes user
     * interactions with UI on the screen such as the notification shade. This does not include
     * WakeReason.TAP or WakeReason.LIFT.
     */
    GESTURE,

    /**
     * Waking up because a wake key other than power was pressed.
     */
    WAKE_KEY,

    /**
     * Waking up because a wake motion was performed.
     */
    WAKE_MOTION,

    /**
     * Waking due to the lid being opened.
     */
    LID,

    /**
     * Waking due to display group being added.
     */
    DISPLAY_GROUP_ADDED,

    /**
     * Waking up due to the user single or double tapping on the screen. This
     * wake reason is used when the user is not tapping on a specific UI element; rather, the device
     * wakes up due to a generic tap on the screen.
     */
    TAP,

    /**
     * Waking up due to a user performed lift gesture.
     */
    LIFT,

    /**
     * Waking up due to a user interacting with a biometric.
     */
    BIOMETRIC,
}
