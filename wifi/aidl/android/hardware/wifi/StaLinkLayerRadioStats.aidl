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

import android.hardware.wifi.WifiChannelStats;

@VintfStability
parcelable StaLinkLayerRadioStats {
    /**
     * Time for which the radio is awake.
     */
    int onTimeInMs;
    /**
     * Total time for which the radio is in active transmission.
     */
    int txTimeInMs;
    /**
     * Time for which the radio is in active tranmission per tx level.
     */
    int[] txTimeInMsPerLevel;
    /**
     * Time for which the radio is in active receive.
     */
    int rxTimeInMs;
    /**
     *  Total time for which the radio is awake due to scan.
     */
    int onTimeInMsForScan;
    /**
     * Total time for which the radio is awake due to NAN scan since boot or crash.
     */
    int onTimeInMsForNanScan;
    /**
     * Total time for which the radio is awake due to background scan since boot or crash.
     */
    int onTimeInMsForBgScan;
    /**
     * Total time for which the radio is awake due to roam scan since boot or crash.
     */
    int onTimeInMsForRoamScan;
    /**
     * Total time for which the radio is awake due to PNO scan since boot or crash.
     */
    int onTimeInMsForPnoScan;
    /**
     * Total time for which the radio is awake due to Hotspot 2.0 scans and GAS exchange since boot
     * or crash.
     */
    int onTimeInMsForHs20Scan;
    /**
     * List of channel stats associated with this radio.
     */
    WifiChannelStats[] channelStats;
    /**
     * Radio ID: An implementation specific value identifying the radio interface for which the
     * stats are produced. Framework must not interpret this value. It must use this value for
     * persistently identifying the statistics between calls,
     * e.g. if the HAL provides them in different order.
     */
    int radioId;
}
