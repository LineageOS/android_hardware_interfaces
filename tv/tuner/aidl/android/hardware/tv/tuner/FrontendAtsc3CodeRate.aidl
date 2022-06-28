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
 * Code Rate for ATSC3.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum FrontendAtsc3CodeRate {
    UNDEFINED = 0,

    /**
     * hardware is able to detect and set Coderate automatically
     */
    AUTO = 1 << 0,

    CODERATE_2_15 = 1 << 1,

    CODERATE_3_15 = 1 << 2,

    CODERATE_4_15 = 1 << 3,

    CODERATE_5_15 = 1 << 4,

    CODERATE_6_15 = 1 << 5,

    CODERATE_7_15 = 1 << 6,

    CODERATE_8_15 = 1 << 7,

    CODERATE_9_15 = 1 << 8,

    CODERATE_10_15 = 1 << 9,

    CODERATE_11_15 = 1 << 10,

    CODERATE_12_15 = 1 << 11,

    CODERATE_13_15 = 1 << 12,
}
