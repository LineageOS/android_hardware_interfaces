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
 * Inner Forward Error Correction type as specified in ETSI EN 300 468 V1.15.1
 * and ETSI EN 302 307-2 V1.1.1.
 * @hide
 */
@VintfStability
@Backing(type="long")
enum FrontendInnerFec {
    /**
     * Not defined
     */
    FEC_UNDEFINED = 0,

    /**
     * hardware is able to detect and set FEC automatically
     */
    AUTO = 1L << 0,

    /**
     * 1/2 conv. code rate
     */
    FEC_1_2 = 1L << 1,

    /**
     * 1/3 conv. code rate
     */
    FEC_1_3 = 1L << 2,

    /**
     * 1/4 conv. code rate
     */
    FEC_1_4 = 1L << 3,

    /**
     * 1/5 conv. code rate
     */
    FEC_1_5 = 1L << 4,

    /**
     * 2/3 conv. code rate
     */
    FEC_2_3 = 1L << 5,

    /**
     * 2/5 conv. code rate
     */
    FEC_2_5 = 1L << 6,

    /**
     * 2/9 conv. code rate
     */
    FEC_2_9 = 1L << 7,

    /**
     * 3/4 conv. code rate
     */
    FEC_3_4 = 1L << 8,

    /**
     * 3/5 conv. code rate
     */
    FEC_3_5 = 1L << 9,

    /**
     * 4/5 conv. code rate
     */
    FEC_4_5 = 1L << 10,

    /**
     * 4/15 conv. code rate
     */
    FEC_4_15 = 1L << 11,

    /**
     * 5/6 conv. code rate
     */
    FEC_5_6 = 1L << 12,

    /**
     * 5/9 conv. code rate
     */
    FEC_5_9 = 1L << 13,

    /**
     * 6/7 conv. code rate
     */
    FEC_6_7 = 1L << 14,

    /**
     * 7/8 conv. code rate
     */
    FEC_7_8 = 1L << 15,

    /**
     * 7/9 conv. code rate
     */
    FEC_7_9 = 1L << 16,

    /**
     * 7/15 conv. code rate
     */
    FEC_7_15 = 1L << 17,

    /**
     * 8/9 conv. code rate
     */
    FEC_8_9 = 1L << 18,

    /**
     * 8/15 conv. code rate
     */
    FEC_8_15 = 1L << 19,

    /**
     * 9/10 conv. code rate
     */
    FEC_9_10 = 1L << 20,

    /**
     * 9/20 conv. code rate
     */
    FEC_9_20 = 1L << 21,

    /**
     * 11/15 conv. code rate
     */
    FEC_11_15 = 1L << 22,

    /**
     * 11/20 conv. code rate
     */
    FEC_11_20 = 1L << 23,

    /**
     * 11/45 conv. code rate
     */
    FEC_11_45 = 1L << 24,

    /**
     * 13/18 conv. code rate
     */
    FEC_13_18 = 1L << 25,

    /**
     * 13/45 conv. code rate
     */
    FEC_13_45 = 1L << 26,

    /**
     * 14/45 conv. code rate
     */
    FEC_14_45 = 1L << 27,

    /**
     * 23/36 conv. code rate
     */
    FEC_23_36 = 1L << 28,

    /**
     * 25/36 conv. code rate
     */
    FEC_25_36 = 1L << 29,

    /**
     * 26/45 conv. code rate
     */
    FEC_26_45 = 1L << 30,

    /**
     * 28/45 conv. code rate
     */
    FEC_28_45 = 1L << 31,

    /**
     * 29/45 conv. code rate
     */
    FEC_29_45 = 1L << 32,

    /**
     * 31/45 conv. code rate
     */
    FEC_31_45 = 1L << 33,

    /**
     * 32/45 conv. code rate
     */
    FEC_32_45 = 1L << 34,

    /**
     * 77/90 conv. code rate
     */
    FEC_77_90 = 1L << 35,

    /**
     * 2/15 conv. code rate
     */
    FEC_2_15 = 1L << 36,

    /**
     * 3/15 conv. code rate
     */
    FEC_3_15 = 1L << 37,

    /**
     * 5/15 conv. code rate
     */
    FEC_5_15 = 1L << 38,

    /**
     * 6/15 conv. code rate
     */
    FEC_6_15 = 1L << 39,

    /**
     * 9/15 conv. code rate
     */
    FEC_9_15 = 1L << 40,

    /**
     * 10/15 conv. code rate
     */
    FEC_10_15 = 1L << 41,

    /**
     * 12/15 conv. code rate
     */
    FEC_12_15 = 1L << 42,

    /**
     * 13/15 conv. code rate
     */
    FEC_13_15 = 1L << 43,

    /**
     * 18/30 conv. code rate
     */
    FEC_18_30 = 1L << 44,

    /**
     * 20/30 conv. code rate
     */
    FEC_20_30 = 1L << 45,

    /**
     * 90/180 conv. code rate
     */
    FEC_90_180 = 1L << 46,

    /**
     * 96/180 conv. code rate
     */
    FEC_96_180 = 1L << 47,

    /**
     * 104/180 conv. code rate
     */
    FEC_104_180 = 1L << 48,

    /**
     * 128/180 conv. code rate
     */
    FEC_128_180 = 1L << 49,

    /**
     * 132/180 conv. code rate
     */
    FEC_132_180 = 1L << 50,

    /**
     * 135/180 conv. code rate
     */
    FEC_135_180 = 1L << 51,

    /**
     * 140/180 conv. code rate
     */
    FEC_140_180 = 1L << 52,
}
