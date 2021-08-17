/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.hardware.biometrics.fingerprint;

@VintfStability
@Backing(type="byte")
enum AcquiredInfo {
    /**
     * Placeholder value used for default initialization of AcquiredInfo. This
     * value means AcquiredInfo wasn't explicitly initialized and must be
     * discarded by the recipient.
     */
    UNKNOWN,

    /**
     * A high quality fingerprint image was detected, no further user interaction is necessary.
     */
    GOOD,

    /**
     * Not enough of a fingerprint was detected. Reposition the finger, or a longer swipe needed.
     */
    PARTIAL,

    /**
     * Image doesn't contain enough detail for recognition.
     */
    INSUFFICIENT,

    /**
     * The sensor needs to be cleaned.
     */
    SENSOR_DIRTY,

    /**
     * For swipe-type sensors, the swipe was too slow and not enough data was collected.
     */
    TOO_SLOW,

    /**
     * For swipe-type sensors, the swipe was too fast and not enough data was collected.
     */
    TOO_FAST,

    /**
     * Vendor-specific acquisition message. See ISessionCallback#onAcquired vendorCode
     * documentation.
     */
    VENDOR,

    /**
     * This message represents the earliest message sent at the beginning of the authentication
     * pipeline. It is expected to be used to measure latency. For example, in a camera-based
     * authentication system it's expected to be sent prior to camera initialization. Note this
     * should be sent whenever authentication is started or restarted. The framework may measure
     * latency based on the time between the last START message and the onAuthenticated callback.
     */
    START,

    /**
     * For sensors that require illumination, such as optical under-display fingerprint sensors,
     * the image was too dark to be used for matching.
     */
    TOO_DARK,

    /**
     * For sensors that require illumination, such as optical under-display fingerprint sensors,
     * the image was too bright to be used for matching.
     */
    TOO_BRIGHT,

    /**
     * This message may be sent during enrollment if the same area of the finger has already
     * been captured during this enrollment session. In general, enrolling multiple areas of the
     * same finger can help against false rejections.
     */
    IMMOBILE,

    /**
     * This message may be sent to notify the framework that an additional image capture is taking
     * place. Multiple RETRYING_CAPTURE may be sent before an ACQUIRED_GOOD message is sent.
     * However, RETRYING_CAPTURE must not be sent after ACQUIRED_GOOD is sent.
     */
    RETRYING_CAPTURE,
}
