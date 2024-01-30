/*
 * Copyright (C) 2023 The Android Open Source Project
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

import android.hardware.wifi.WifiChannelWidthInMhz;
import android.hardware.wifi.WifiRatePreamble;

/**
 * Scan result cached in Wifi firmware
 */
@VintfStability
parcelable CachedScanResult {
    /**
     * Time in micro seconds since boot when the scan was done
     */
    long timeStampInUs;
    /**
     * SSID of beacon excluding null.
     */
    byte[] ssid;
    /**
     * BSSID of beacon
     */
    byte[6] bssid;
    /**
     * Beacon received signal stength indicatior (RSSI), in dbm
     */
    int rssiDbm;
    /**
     * Frequency of beacon, in MHz
     */
    int frequencyMhz;
    /**
     * Channel bandwidth of found network
     */
    WifiChannelWidthInMhz channelWidthMhz;
    /**
     * Supported rate and preamble type
     */
    WifiRatePreamble preambleType;
}
