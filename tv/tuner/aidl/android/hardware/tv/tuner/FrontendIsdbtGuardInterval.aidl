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
 * Guard Interval Type for ISDBT.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum FrontendIsdbtGuardInterval {
    UNDEFINED = 0,

    /**
     * hardware is able to detect and set Guard Interval automatically
     */
    AUTO = 1 << 0,

    INTERVAL_1_32 = 1 << 1,

    INTERVAL_1_16 = 1 << 2,

    INTERVAL_1_8 = 1 << 3,

    INTERVAL_1_4 = 1 << 4,

    INTERVAL_1_128 = 1 << 5,

    INTERVAL_19_128 = 1 << 6,

    INTERVAL_19_256 = 1 << 7,
}
