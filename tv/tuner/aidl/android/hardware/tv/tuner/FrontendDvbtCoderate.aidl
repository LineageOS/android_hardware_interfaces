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
 * Code Rate for DVBT.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum FrontendDvbtCoderate {
    UNDEFINED = 0,

    /**
     * hardware is able to detect and set Hierarchy automatically
     */
    AUTO = 1 << 0,

    CODERATE_1_2 = 1 << 1,

    CODERATE_2_3 = 1 << 2,

    CODERATE_3_4 = 1 << 3,

    CODERATE_5_6 = 1 << 4,

    CODERATE_7_8 = 1 << 5,

    CODERATE_3_5 = 1 << 6,

    CODERATE_4_5 = 1 << 7,

    CODERATE_6_7 = 1 << 8,

    CODERATE_8_9 = 1 << 9,
}
