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
 * Time Interleave Mode for ISDB-T Frontend.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum FrontendIsdbtTimeInterleaveMode {
    UNDEFINED = 0,

    /**
     * Hardware is able to detect and set Time Interleave Mode automatically.
     */
    AUTO = 1 << 0,

    INTERLEAVE_1_0 = 1 << 1,

    INTERLEAVE_1_4 = 1 << 2,

    INTERLEAVE_1_8 = 1 << 3,

    INTERLEAVE_1_16 = 1 << 4,

    INTERLEAVE_2_0 = 1 << 5,

    INTERLEAVE_2_2 = 1 << 6,

    INTERLEAVE_2_4 = 1 << 7,

    INTERLEAVE_2_8 = 1 << 8,

    INTERLEAVE_3_0 = 1 << 9,

    INTERLEAVE_3_1 = 1 << 10,

    INTERLEAVE_3_2 = 1 << 11,

    INTERLEAVE_3_4 = 1 << 12,
}
