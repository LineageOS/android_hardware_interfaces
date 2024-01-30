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

package android.hardware.radio.ims.media;

/**
 * EVS codec mode to represent the bit rate. See 3ggp Spec 26.952 Table 5.1
 * @hide
 */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum EvsMode {
    /** 6.6 kbps for EVS AMR-WB IO */
    EVS_MODE_0 = 1 << 0,
    /** 8.855 kbps for AMR-WB IO */
    EVS_MODE_1 = 1 << 1,
    /** 12.65 kbps for AMR-WB IO */
    EVS_MODE_2 = 1 << 2,
    /** 14.25 kbps for AMR-WB IO */
    EVS_MODE_3 = 1 << 3,
    /** 15.85 kbps for AMR-WB IO */
    EVS_MODE_4 = 1 << 4,
    /** 18.25 kbps for AMR-WB IO */
    EVS_MODE_5 = 1 << 5,
    /** 19.85 kbps for AMR-WB IO */
    EVS_MODE_6 = 1 << 6,
    /** 23.05 kbps for AMR-WB IO */
    EVS_MODE_7 = 1 << 7,
    /** 23.85 kbps for AMR-WB IO */
    EVS_MODE_8 = 1 << 8,
    /** 5.9 kbps for EVS primary */
    EVS_MODE_9 = 1 << 9,
    /** 7.2 kbps for EVS primary */
    EVS_MODE_10 = 1 << 10,
    /** 8.0 kbps for EVS primary */
    EVS_MODE_11 = 1 << 11,
    /** 9.6 kbps for EVS primary */
    EVS_MODE_12 = 1 << 12,
    /** 13.2 kbps for EVS primary */
    EVS_MODE_13 = 1 << 13,
    /** 16.4 kbps for EVS primary */
    EVS_MODE_14 = 1 << 14,
    /** 24.4 kbps for EVS primary */
    EVS_MODE_15 = 1 << 15,
    /** 32.0 kbps for EVS primary */
    EVS_MODE_16 = 1 << 16,
    /** 48.0 kbps for EVS primary */
    EVS_MODE_17 = 1 << 17,
    /** 64.0 kbps for EVS primary */
    EVS_MODE_18 = 1 << 18,
    /** 96.0 kbps for EVS primary */
    EVS_MODE_19 = 1 << 19,
    /** 128.0 kbps for EVS primary */
    EVS_MODE_20 = 1 << 20,
}
