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

import android.hardware.wifi.StaLinkLayerIfaceContentionTimeStats;
import android.hardware.wifi.StaLinkLayerIfacePacketStats;
import android.hardware.wifi.StaPeerInfo;

/**
 * Iface statistics for the current connection.
 */
@VintfStability
parcelable StaLinkLayerIfaceStats {
    /**
     * Number beacons received from the connected AP.
     */
    int beaconRx;
    /**
     * Access Point Beacon and Management frames RSSI (averaged).
     */
    int avgRssiMgmt;
    /**
     * WME Best Effort Access Category packet counters.
     */
    StaLinkLayerIfacePacketStats wmeBePktStats;
    /**
     * WME Background Access Category packet counters.
     */
    StaLinkLayerIfacePacketStats wmeBkPktStats;
    /**
     * WME Video Access Category packet counters.
     */
    StaLinkLayerIfacePacketStats wmeViPktStats;
    /**
     * WME Voice Access Category packet counters.
     */
    StaLinkLayerIfacePacketStats wmeVoPktStats;
    /**
     * Duty cycle for the iface.
     * If this iface is being served using time slicing on a radio with one or more ifaces
     * (i.e MCC), then the duty cycle assigned to this iface in %.
     * If not using time slicing (i.e SCC or DBS), set to 100.
     */
    byte timeSliceDutyCycleInPercent;
    /**
     * WME Best Effort (BE) Access Category (AC) contention time statistics.
     */
    StaLinkLayerIfaceContentionTimeStats wmeBeContentionTimeStats;
    /**
     * WME Background (BK) Access Category (AC) contention time statistics.
     */
    StaLinkLayerIfaceContentionTimeStats wmeBkContentionTimeStats;
    /**
     * WME Video (VI) Access Category (AC) contention time statistics.
     */
    StaLinkLayerIfaceContentionTimeStats wmeViContentionTimeStats;
    /**
     * WME Voice (VO) Access Category (AC) contention time statistics.
     */
    StaLinkLayerIfaceContentionTimeStats wmeVoContentionTimeStats;
    /**
     * Per peer statistics.
     */
    StaPeerInfo[] peers;
}
