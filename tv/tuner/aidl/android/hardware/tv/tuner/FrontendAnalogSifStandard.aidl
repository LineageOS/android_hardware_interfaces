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
 * Standard Interchange Format (SIF) for Analog Frontend.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum FrontendAnalogSifStandard {
    UNDEFINED = 0,

    AUTO = 1 << 0,

    BG = 1 << 1,

    BG_A2 = 1 << 2,

    BG_NICAM = 1 << 3,

    I = 1 << 4,

    DK = 1 << 5,

    DK1_A2 = 1 << 6,

    DK2_A2 = 1 << 7,

    DK3_A2 = 1 << 8,

    DK_NICAM = 1 << 9,

    L = 1 << 10,

    M = 1 << 11,

    M_BTSC = 1 << 12,

    M_A2 = 1 << 13,

    M_EIAJ = 1 << 14,

    I_NICAM = 1 << 15,

    L_NICAM = 1 << 16,

    L_PRIME = 1 << 17,
}
