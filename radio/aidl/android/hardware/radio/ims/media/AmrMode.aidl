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
 * AMR codec mode to represent the bit rate. See 3ggp Specs 26.976 & 26.071
 * @hide
 */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum AmrMode {
    /** 4.75 kbps for AMR / 6.6 kbps for AMR-WB */
    AMR_MODE_0 = 1 << 0,
    /** 5.15 kbps for AMR / 8.855 kbps for AMR-WB */
    AMR_MODE_1 = 1 << 1,
    /** 5.9 kbps for AMR / 12.65 kbps for AMR-WB */
    AMR_MODE_2 = 1 << 2,
    /** 6.7 kbps for AMR / 14.25 kbps for AMR-WB */
    AMR_MODE_3 = 1 << 3,
    /** 7.4 kbps for AMR / 15.85 kbps for AMR-WB */
    AMR_MODE_4 = 1 << 4,
    /** 7.95 kbps for AMR / 18.25 kbps for AMR-WB */
    AMR_MODE_5 = 1 << 5,
    /** 10.2 kbps for AMR / 19.85 kbps for AMR-WB */
    AMR_MODE_6 = 1 << 6,
    /** 12.2 kbps for AMR / 23.05 kbps for AMR-WB */
    AMR_MODE_7 = 1 << 7,
    /** Silence frame for AMR / 23.85 kbps for AMR-WB */
    AMR_MODE_8 = 1 << 8,
}
