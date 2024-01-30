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

package android.hardware.radio.network;

/**
 * UTRAN bands up to V15.0.0
 * @hide
 */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum UtranBands {
    BAND_1 = 1,
    BAND_2 = 2,
    BAND_3 = 3,
    BAND_4 = 4,
    BAND_5 = 5,
    BAND_6 = 6,
    BAND_7 = 7,
    BAND_8 = 8,
    BAND_9 = 9,
    BAND_10 = 10,
    BAND_11 = 11,
    BAND_12 = 12,
    BAND_13 = 13,
    BAND_14 = 14,
    BAND_19 = 19,
    BAND_20 = 20,
    BAND_21 = 21,
    BAND_22 = 22,
    BAND_25 = 25,
    BAND_26 = 26,
    /**
     * TD-SCDMA bands. 3GPP TS 25.102, Table 5.2: Frequency bands
     */
    BAND_A = 101,
    BAND_B = 102,
    BAND_C = 103,
    BAND_D = 104,
    BAND_E = 105,
    BAND_F = 106,
}
