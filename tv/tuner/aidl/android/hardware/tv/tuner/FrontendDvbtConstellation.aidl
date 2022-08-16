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
 * Extended Constellation for DVBT.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum FrontendDvbtConstellation {
    UNDEFINED = 0,

    /**
     * hardware is able to detect and set Constellation automatically
     */
    AUTO = 1 << 0,

    CONSTELLATION_QPSK = 1 << 1,

    CONSTELLATION_16QAM = 1 << 2,

    CONSTELLATION_64QAM = 1 << 3,

    CONSTELLATION_256QAM = 1 << 4,

    CONSTELLATION_QPSK_R = 1 << 5,

    CONSTELLATION_16QAM_R = 1 << 6,

    CONSTELLATION_64QAM_R = 1 << 7,

    CONSTELLATION_256QAM_R = 1 << 8,
}
