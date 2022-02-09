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

package android.hardware.wifi.hostapd;

/**
 * The channel bandwidth of the AP.
 */
@VintfStability
@Backing(type="int")
enum ChannelBandwidth {
    /**
     * Invalid bandwidth value for AP
     */
    BANDWIDTH_INVALID = 0,
    /**
     * Channel bandwidth is auto-selected by the chip
     */
    BANDWIDTH_AUTO = 1,
    /**
     * AP channel bandwidth is 20 MHz but not HT
     */
    BANDWIDTH_20_NOHT = 2,
    /**
     * AP channel bandwidth is 20 MHz
     */
    BANDWIDTH_20 = 3,
    /**
     * AP channel bandwidth is 40 MHz
     */
    BANDWIDTH_40 = 4,
    /**
     * AP channel bandwidth is 80 MHz
     */
    BANDWIDTH_80 = 5,
    /**
     * AP channel bandwidth is 80+80 MHz
     */
    BANDWIDTH_80P80 = 6,
    /**
     * AP channel bandwidth is 160 MHz
     */
    BANDWIDTH_160 = 7,
    /**
     * AP channel bandwidth is 320 MHz
     */
    BANDWIDTH_320 = 8,
    /**
     * AP channel bandwidth is 2160 MHz
     */
    BANDWIDTH_2160 = 9,
    /**
     * AP channel bandwidth is 4320 MHz
     */
    BANDWIDTH_4320 = 10,
    /**
     * AP channel bandwidth is 6480 MHz
     */
    BANDWIDTH_6480 = 11,
    /**
     * AP channel bandwidth is 8640 MHz
     */
    BANDWIDTH_8640 = 12,
}
