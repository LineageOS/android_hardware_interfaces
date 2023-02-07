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
 * Per Link statistics for the current connection. For MLO, this is
 * the statistics for one link in the connection.
 */
@VintfStability
parcelable StaLinkLayerLinkStats {
    /**
     * Identifier for the link within MLO. For single link operation this field
     * is not relevant and can be set to 0.
     */
    int linkId;
    /**
     * Radio identifier on which the link is currently operating. Refer
     * |StaLinkLayerRadioStats.radioId|.
     */
    int radioId;
    /**
     * Frequency of the link in Mhz.
     */
    int frequencyMhz;
    /**
     * Number of beacons received from the connected AP on the link.
     */
    int beaconRx;
    /**
     * Access Point Beacon and Management frames RSSI (averaged) on the link.
     */
    int avgRssiMgmt;
    /**
     * WME Best Effort Access Category packet counters on the link.
     */
    StaLinkLayerIfacePacketStats wmeBePktStats;
    /**
     * WME Background Access Category packet counters on the link.
     */
    StaLinkLayerIfacePacketStats wmeBkPktStats;
    /**
     * WME Video Access Category packet counters on the link.
     */
    StaLinkLayerIfacePacketStats wmeViPktStats;
    /**
     * WME Voice Access Category packet counters on the link.
     */
    StaLinkLayerIfacePacketStats wmeVoPktStats;
    /**
     * Duty cycle for the link.
     * If this link is being served using time slicing on a radio with one or
     * more links then the duty cycle assigned to this link in %. If not using
     * time slicing, set to 100.
     */
    byte timeSliceDutyCycleInPercent;
    /**
     * WME Best Effort (BE) Access Category (AC) contention time statistics on
     * the link.
     */
    StaLinkLayerIfaceContentionTimeStats wmeBeContentionTimeStats;
    /**
     * WME Background (BK) Access Category (AC) contention time statistics on
     * the link.
     */
    StaLinkLayerIfaceContentionTimeStats wmeBkContentionTimeStats;
    /**
     * WME Video (VI) Access Category (AC) contention time statistics on the
     * link.
     */
    StaLinkLayerIfaceContentionTimeStats wmeViContentionTimeStats;
    /**
     * WME Voice (VO) Access Category (AC) contention time statistics on the
     * link.
     */
    StaLinkLayerIfaceContentionTimeStats wmeVoContentionTimeStats;
    /**
     * Per peer statistics for the link.
     */
    StaPeerInfo[] peers;
    /**
     * Various states of the link.
     */
    @Backing(type="int")
    @VintfStability
    enum StaLinkState {
        /**
         * Chip does not support reporting the state of the link.
         */
        UNKNOWN = 0,
        /**
         * Link has not been in use since last report. It is placed in power save. All management,
         * control and data frames for the MLO connection are carried over other links. In this
         * state the link will not listen to beacons even in DTIM period and does not perform any
         * GTK/IGTK/BIGTK updates but remains associated.
         */
        NOT_IN_USE = 1 << 0,
        /**
         * Link is in use. In presence of traffic, it is set to be power active. When the traffic
         * stops, the link will go into power save mode and will listen for beacons every DTIM
         * period.
         *
         */
        IN_USE = 1 << 1,
    }
    /**
     * State of the link. Refer |StaLinkState|.
     */
    StaLinkState state;
}
