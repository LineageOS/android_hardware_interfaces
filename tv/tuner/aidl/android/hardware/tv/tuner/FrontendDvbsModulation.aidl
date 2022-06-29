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
 * Modulation Type for DVBS.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum FrontendDvbsModulation {
    UNDEFINED = 0,

    /**
     * hardware is able to detect and set Modulation automatically
     */
    AUTO = 1 << 0,

    MOD_QPSK = 1 << 1,

    MOD_8PSK = 1 << 2,

    MOD_16QAM = 1 << 3,

    MOD_16PSK = 1 << 4,

    MOD_32PSK = 1 << 5,

    MOD_ACM = 1 << 6,

    MOD_8APSK = 1 << 7,

    MOD_16APSK = 1 << 8,

    MOD_32APSK = 1 << 9,

    MOD_64APSK = 1 << 10,

    MOD_128APSK = 1 << 11,

    MOD_256APSK = 1 << 12,

    /**
     * Reserved for Proprietary modulation
     */
    MOD_RESERVED = 1 << 13,
}
