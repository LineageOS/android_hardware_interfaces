/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.wifi;

/**
 * Wifi Rate Preamble.
 */
@VintfStability
@Backing(type="int")
enum WifiRatePreamble {
    OFDM = 0,
    CCK = 1,
    HT = 2,
    VHT = 3,
    RESERVED = 4,
    /**
     * Preamble type for 11ax.
     */
    HE = 5,
    /**
     * Preamble type for 11be.
     */
    EHT = 6,
}
