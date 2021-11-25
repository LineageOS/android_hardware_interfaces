/*
 * Copyright 2021 The Android Open Source Project
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
 * Indexes can be tagged by NAL unit group in HEVC according to ISO/IEC 23008-2.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum DemuxScHevcIndex {
    UNDEFINED = 0,

    SPS = 1 << 0,

    AUD = 1 << 1,

    SLICE_CE_BLA_W_LP = 1 << 2,

    SLICE_BLA_W_RADL = 1 << 3,

    SLICE_BLA_N_LP = 1 << 4,

    SLICE_IDR_W_RADL = 1 << 5,

    SLICE_IDR_N_LP = 1 << 6,

    SLICE_TRAIL_CRA = 1 << 7,
}
