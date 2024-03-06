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

package android.hardware.biometrics.face;

/**
 * Enrollment stages that can be mapped to the enrollment UI actions in the framework.
 * @hide
 */
@VintfStability
@Backing(type="byte")
enum EnrollmentStage {
    /**
     * Placeholder value used for default initialization of EnrollmentStage. This
     * value means EnrollmentStage wasn't explicitly initialized and must be
     * discarded by the recipient.
     */
    UNKNOWN,

    /**
     * HAL has obtained the first camera frame.
     */
    FIRST_FRAME_RECEIVED,

    /**
     * HAL is waiting for the user's face to be centered.
     */
    WAITING_FOR_CENTERING,

    /**
     * HAL is expecting the user's face to stay centered.
     */
    HOLD_STILL_IN_CENTER,

    /**
     * Vendor defined movement 1.
     */
    ENROLLING_MOVEMENT_1,

    /**
     * Vendor defined movement 2.
     */
    ENROLLING_MOVEMENT_2,

    /**
     * HAL has finished the enrollment.
     */
    ENROLLMENT_FINISHED,
}
