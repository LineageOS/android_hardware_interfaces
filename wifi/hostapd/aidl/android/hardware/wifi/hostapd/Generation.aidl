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
 * The wifi operational mode of the AP.
 * It depends on hw mode and HT/VHT capabilities in hostapd.
 *
 * WIFI_STANDARD_LEGACY = (hw_mode is HOSTAPD_MODE_IEEE80211B) or
 *                        (hw_mode is HOSTAPD_MODE_IEEE80211G and HT is 0).
 * WIFI_STANDARD_11N = [hw_mode is HOSTAPD_MODE_IEEE80211G and (HT is 1 or HT40 is 1)] or
 *                     [hw_mode is HOSTAPD_MODE_IEEE80211A and VHT is 0].
 * WIFI_STANDARD_11AC = hw_mode is HOSTAPD_MODE_IEEE80211A and VHT is 1.
 * WIFI_STANDARD_11AD = hw_mode is HOSTAPD_MODE_IEEE80211AD.
 * WIFI_STANDARD_11AX = hw_mode is HOSTAPD_MODE_IEEE80211A and High Efficiency supported.
 * WIFI_STANDARD_11BE = hw_mode is HOSTAPD_MODE_IEEE80211A and Extreme High Throughput supported.
 */
@VintfStability
@Backing(type="int")
enum Generation {
    WIFI_STANDARD_UNKNOWN = -1,
    WIFI_STANDARD_LEGACY = 0,
    WIFI_STANDARD_11N = 1,
    WIFI_STANDARD_11AC = 2,
    WIFI_STANDARD_11AD = 3,
    WIFI_STANDARD_11AX = 4,
    WIFI_STANDARD_11BE = 5,
}
