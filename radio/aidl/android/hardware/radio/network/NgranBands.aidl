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
 * NGRAN bands up to V16.5.0
 * @hide
 */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum NgranBands {
    /**
     * 3GPP TS 38.101-1, Table 5.2-1: FR1 bands
     */
    BAND_1 = 1,
    BAND_2 = 2,
    BAND_3 = 3,
    BAND_5 = 5,
    BAND_7 = 7,
    BAND_8 = 8,
    BAND_12 = 12,
    BAND_14 = 14,
    BAND_18 = 18,
    BAND_20 = 20,
    BAND_25 = 25,
    BAND_26 = 26,
    BAND_28 = 28,
    BAND_29 = 29,
    BAND_30 = 30,
    BAND_34 = 34,
    BAND_38 = 38,
    BAND_39 = 39,
    BAND_40 = 40,
    BAND_41 = 41,
    BAND_46 = 46,
    BAND_48 = 48,
    BAND_50 = 50,
    BAND_51 = 51,
    BAND_53 = 53,
    BAND_65 = 65,
    BAND_66 = 66,
    BAND_70 = 70,
    BAND_71 = 71,
    BAND_74 = 74,
    BAND_75 = 75,
    BAND_76 = 76,
    BAND_77 = 77,
    BAND_78 = 78,
    BAND_79 = 79,
    BAND_80 = 80,
    BAND_81 = 81,
    BAND_82 = 82,
    BAND_83 = 83,
    BAND_84 = 84,
    BAND_86 = 86,
    BAND_89 = 89,
    BAND_90 = 90,
    BAND_91 = 91,
    BAND_92 = 92,
    BAND_93 = 93,
    BAND_94 = 94,
    BAND_95 = 95,
    BAND_96 = 96,
    /**
     * 3GPP TS 38.101-2, Table 5.2-1: FR2 bands
     */
    BAND_257 = 257,
    BAND_258 = 258,
    BAND_260 = 260,
    BAND_261 = 261,
}
