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

import android.hardware.wifi.WifiChannelWidthInMhz;
import android.hardware.wifi.WifiRateNss;
import android.hardware.wifi.WifiRatePreamble;

/**
 * Wifi rate info.
 */
@VintfStability
parcelable WifiRateInfo {
    /**
     * Preamble used for RTT measurements.
     */
    WifiRatePreamble preamble;
    /**
     * Number of spatial streams.
     */
    WifiRateNss nss;
    /**
     * Bandwidth of channel.
     */
    WifiChannelWidthInMhz bw;
    /**
     * OFDM/CCK rate code as per IEEE std in units of 0.5mbps.
     * HT/VHT/HE/EHT would be mcs index.
     */
    byte rateMcsIdx;
    /**
     * Bitrate in units of 100 Kbps.
     */
    int bitRateInKbps;
}
