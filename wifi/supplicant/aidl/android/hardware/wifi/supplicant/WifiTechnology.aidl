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

package android.hardware.wifi.supplicant;

/**
 * Wifi Technologies
 */
@VintfStability
@Backing(type="int")
enum WifiTechnology {
    UNKNOWN = 0,
    /**
     * For 802.11a/b/g
     */
    LEGACY = 1,
    /**
     * For 802.11n
     */
    HT = 2,
    /**
     * For 802.11ac
     */
    VHT = 3,
    /**
     * For 802.11ax
     */
    HE = 4,
    /**
     * For 802.11be
     */
    EHT = 5,
}
