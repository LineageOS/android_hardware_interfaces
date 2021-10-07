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

import android.hardware.biometrics.face.AcquiredInfo;

/**
 * Metadata of an individual frame. Can be used by the framework to provide user feedback.
 * This parcelable is part of AuthenticationFrame and EnrollmentFrame, and shouldn't be used
 * independently of those parcelables.
 */
@VintfStability
parcelable BaseFrame {
    /**
     * Information about the frame that can be used by the framework to provide feedback to the
     * user, for example ask the user to move their face in a certain way.
     */
    AcquiredInfo acquiredInfo = AcquiredInfo.UNKNOWN;

    /**
     * If acquiredInfo is set to AcquiredInfo::VENDOR. This is the index into the configuration
     * "com.android.internal.R.array.face_acquired_vendor" that's installed on the vendor partition.
     * Otherwise, this value must be ignored.
     */
    int vendorCode;

    /**
     * Pan value. It is recommended to use the range of [-1, 1] to represent valid values, and
     * anything outside of that range to represent errors. However, vendors are free to define
     * their own way of representing valid values and errors.
     */
    float pan;

    /**
     * Tilt value. It is recommended to use the range of [-1, 1] to represent valid values, and
     * anything outside of that range to represent errors. However, vendors are free to define
     * their own way of representing valid values and errors.
     */
    float tilt;

    /**
     * Distance value. It is recommended to use the range of [-1, 1] to represent valid values, and
     * anything outside of that range to represent errors. However, vendors are free to define
     * their own way of representing valid values and errors.
     */
    float distance;

    /**
     * Indicates that the HAL can no longer continue with authentication or enrollment. This allows
     * the framework to correlate a failure condition with a particular AcquiredInfo, rather than
     * having a sequence of AcquiredInfo + Error.
     */
    boolean isCancellable;
}
