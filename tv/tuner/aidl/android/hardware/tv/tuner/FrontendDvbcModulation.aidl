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
 * Modulation Type for DVBC.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum FrontendDvbcModulation {
    UNDEFINED = 0,

    /**
     * hardware is able to detect and set Modulation automatically
     */
    AUTO = 1 << 0,

    MOD_16QAM = 1 << 1,

    MOD_32QAM = 1 << 2,

    MOD_64QAM = 1 << 3,

    MOD_128QAM = 1 << 4,

    MOD_256QAM = 1 << 5,
}
