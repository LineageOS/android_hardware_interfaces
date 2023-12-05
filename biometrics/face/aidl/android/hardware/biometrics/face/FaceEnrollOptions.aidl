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

package android.hardware.biometrics.face;

import android.hardware.biometrics.common.OperationContext;
import android.hardware.biometrics.face.EnrollmentStageConfig;
import android.hardware.biometrics.face.EnrollmentType;
import android.hardware.biometrics.face.Feature;
import android.hardware.common.NativeHandle;
import android.hardware.keymaster.HardwareAuthToken;
import android.view.Surface;

/**
 * Enroll options used to pass information to the HAL when requesting an enroll operation.
 * @hide
 */
@VintfStability
parcelable FaceEnrollOptions {
    /**
     * See {@link HardwareAuthToken}.
     */
    HardwareAuthToken hardwareAuthToken;

    /**
     * See {@link EnrollmentType}
     */
    EnrollmentType enrollmentType;

    /**
     * See {@link Feature}
     */
    Feature[] features;

    /**
     * @deprecated use {@link surfacePreview} instead
     *
     * {@link NativeHandle} a handle used to render content from the face HAL.
     *
     * Note that only one of [{@link surfacePreview}, {@link nativeHandlePreview}]
     * should be set at one time.
     */
    @nullable NativeHandle nativeHandlePreview;

    /**
     * {@link Surface} a surface used to render content from the face HAL.
     *
     * Note that only one of [{@link surfacePreview}, {@link nativeHandlePreview}]
     * should be set at one time.
     */
    @nullable Surface surfacePreview;

    /**
     * See {@link OperationContext}
     */
    @nullable OperationContext context;
}
