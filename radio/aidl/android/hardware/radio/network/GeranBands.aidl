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

/** @hide */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum GeranBands {
    BAND_T380 = 1,
    BAND_T410 = 2,
    BAND_450 = 3,
    BAND_480 = 4,
    BAND_710 = 5,
    BAND_750 = 6,
    BAND_T810 = 7,
    BAND_850 = 8,
    BAND_P900 = 9,
    BAND_E900 = 10,
    BAND_R900 = 11,
    BAND_DCS1800 = 12,
    BAND_PCS1900 = 13,
    BAND_ER900 = 14,
}
