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

package android.hardware.vibrator;

import android.hardware.vibrator.PwleV2Primitive;

@VintfStability
parcelable CompositePwleV2 {
    /**
     * Represents a PWLE (Piecewise-Linear Envelope) effect as an array of primitives.
     *
     * A PWLE effect defines a vibration waveform using amplitude and frequency points.
     * The envelope linearly interpolates both amplitude and frequency between consecutive points,
     * creating smooth transitions in the vibration pattern.
     */
    PwleV2Primitive[] pwlePrimitives;
}
