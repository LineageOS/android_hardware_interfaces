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
 * Enum values indicating the reason code for disconnect packet.
 * Reason codes (IEEE Std 802.11-2016, 9.4.1.7, Table 9-45).
 */
@VintfStability
@Backing(type="int")
enum Ieee80211ReasonCode {
    WLAN_REASON_UNSPECIFIED = 1,
    WLAN_REASON_PREV_AUTH_NOT_VALID = 2,
    WLAN_REASON_DISASSOC_AP_BUSY = 5,
}
