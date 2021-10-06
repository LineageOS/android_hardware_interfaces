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

import android.hardware.biometrics.face.BaseFrame;
import android.hardware.biometrics.face.Cell;
import android.hardware.biometrics.face.EnrollmentStage;

/**
 * Describes an individual frame captured during enrollment.
 */
@VintfStability
parcelable EnrollmentFrame {
    /**
     * The enrollment UI cell that was captured in this frame, or null if no cell was captured.
     */
    @nullable Cell cell;

    /**
     * The enrollment stage for which this frame was captured.
     */
    EnrollmentStage stage = EnrollmentStage.UNKNOWN;

    /**
     * The frame metadata. Can be used by the framework to provide user feedback.
     */
    BaseFrame data;
}
