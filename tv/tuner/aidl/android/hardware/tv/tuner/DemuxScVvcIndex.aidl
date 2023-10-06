/*
 * Copyright 2022 The Android Open Source Project
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

package android.hardware.tv.tuner;

/**
 * Indexes can be tagged by start point of slice groups according to ISO/IEC 23090-3.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum DemuxScVvcIndex {
    UNDEFINED = 0,

    /**
     * Coded slice of an IDR picture or subpicture with RADL pictures.
     */
    SLICE_IDR_W_RADL = 1 << 0,

    /**
     * Coded slice of an IDR picture or subpicture without leading pictures.
     */
    SLICE_IDR_N_LP = 1 << 1,

    /**
     * Coded slice of a CRA (clean random access) picture or subpicture.
     */
    SLICE_CRA = 1 << 2,

    /**
     * Coded slice of a GDR (gradual decoder refresh) picture or subpicture.
     */
    SLICE_GDR = 1 << 3,

    /**
     * Video parameter set (non-VCL NALU).
     */
    VPS = 1 << 4,

    /**
     * Sequence parameter set (non-VCL NALU).
     */
    SPS = 1 << 5,

    /**
     * Access unit delimiter (non-VCL NALU).
     */
    AUD = 1 << 6,
}
