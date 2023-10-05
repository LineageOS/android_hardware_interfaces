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

package android.hardware.biometrics.fingerprint;

/**
 * @hide
 */
@VintfStability
parcelable TouchDetectionParameters {
    /**
     * The percentage of the sensor that is considered the target. Value is required to be within
     * [0.0, 1.0]. The target area expands outwards from center matching the sensorShape. Some
     * portion of the touch must be within the target to be considered a valid touch.
     */
    float targetSize = 1.0f;

    /**
     * The minimum percentage overlap needed on the sensor to be considered a valid touch. Value is
     * required to be within [0.0, 1.0].
     */
    float minOverlap = 0.0f;
}
