/*
 * Copyright (C) 2024 The Android Open Source Project
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
 * Additional state associated with an operation
 *
 * @hide
 */
@VintfStability
union OperationState {
    /** Operation state related to fingerprint*/
    @VintfStability
    parcelable FingerprintOperationState {
        ParcelableHolder extension;

        /** Flag indicating if the HAL should ignore touches on the fingerprint sensor */
        boolean isHardwareIgnoringTouches = false;
    }

    /** Operation state related to face*/
    @VintfStability
    parcelable FaceOperationState {
        ParcelableHolder extension;
    }

    OperationState.FingerprintOperationState fingerprintOperationState;
    OperationState.FaceOperationState faceOperationState;
}
