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
 * Reason for an authenticate operation.
 *
 * @hide
 */
@VintfStability
union AuthenticateReason {
    /** Vendor reason for invoking an authenticate operation. */
    @VintfStability
    parcelable Vendor {
        ParcelableHolder extension;
    }

    /** Reason for invoking fingerprint authentication. */
    @VintfStability
    @Backing(type="int")
    enum Fingerprint {
        UNKNOWN,
    }

    /** Reason for invoking face authentication. */
    @VintfStability
    @Backing(type="int")
    enum Face {
        UNKNOWN,
        STARTED_WAKING_UP,
        PRIMARY_BOUNCER_SHOWN,
        ASSISTANT_VISIBLE,
        ALTERNATE_BIOMETRIC_BOUNCER_SHOWN,
        NOTIFICATION_PANEL_CLICKED,
        OCCLUDING_APP_REQUESTED,
        PICK_UP_GESTURE_TRIGGERED,
        QS_EXPANDED,
        SWIPE_UP_ON_BOUNCER,
        UDFPS_POINTER_DOWN,
    }

    AuthenticateReason.Vendor vendorAuthenticateReason;
    AuthenticateReason.Face faceAuthenticateReason;
    AuthenticateReason.Fingerprint fingerprintAuthenticateReason;
}
