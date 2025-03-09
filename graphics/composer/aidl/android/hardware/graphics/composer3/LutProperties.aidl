/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.graphics.composer3;

/**
 * The properties of the LUT (Look-Up Table).
 */
@VintfStability
parcelable LutProperties {
    /**
     * The dimension of the Lut.
     * Either 1d or 3d.
     */
    @VintfStability enum Dimension { ONE_D = 1, THREE_D = 3 }
    Dimension dimension;

    /**
     * The size of the Lut.
     * This refers to the length of a 1D Lut, or the grid size of a 3D one.
     */
    int size;

    /**
     * SamplingKey is about how a Lut can be sampled.
     * A Lut can be sampled in more than one key,
     * but only one sampling key is used at one time.
     *
     * The implementations should use a sampling strategy
     * at least as good as linear sampling.
     */
    @VintfStability enum SamplingKey { RGB, MAX_RGB, CIE_Y }
    SamplingKey[] samplingKeys;
}
