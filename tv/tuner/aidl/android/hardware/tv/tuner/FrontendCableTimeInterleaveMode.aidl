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
 * Time Interleave Mode for DVBC Frontend.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum FrontendCableTimeInterleaveMode {
    UNDEFINED = 0,

    AUTO = 1 << 0,

    INTERLEAVING_128_1_0 = 1 << 1,

    INTERLEAVING_128_1_1 = 1 << 2,

    INTERLEAVING_64_2 = 1 << 3,

    INTERLEAVING_32_4 = 1 << 4,

    INTERLEAVING_16_8 = 1 << 5,

    INTERLEAVING_8_16 = 1 << 6,

    INTERLEAVING_128_2 = 1 << 7,

    INTERLEAVING_128_3 = 1 << 8,

    INTERLEAVING_128_4 = 1 << 9,
}
