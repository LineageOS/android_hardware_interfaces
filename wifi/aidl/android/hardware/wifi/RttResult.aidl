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

import android.hardware.wifi.RttBw;
import android.hardware.wifi.RttStatus;
import android.hardware.wifi.RttType;
import android.hardware.wifi.WifiInformationElement;
import android.hardware.wifi.WifiRateInfo;

/**
 * RTT results.
 */
@VintfStability
parcelable RttResult {
    /**
     * Peer device mac address.
     */
    byte[6] addr;
    /**
     * Burst number in a multi-burst request.
     *
     * Note: Applicable to 1-sided RTT and 2-sided IEEE 802.11mc only.
     */
    int burstNum;
    /**
     * Total RTT measurement frames attempted.
     */
    int measurementNumber;
    /**
     * Total successful RTT measurement frames.
     */
    int successNumber;
    /**
     * Maximum number of "FTM frames per burst" supported by
     * the responder STA. Applies to 2-sided IEEE 802.11mc RTT only.
     * If reponder overrides with larger value:
     * - for single-burst request, initiator will truncate the
     * larger value and send a TMR_STOP after receiving as
     * many frames as originally requested.
     * - for multi-burst request, initiator will return
     * failure right away.
     */
    byte numberPerBurstPeer;
    /**
     * Ranging status.
     */
    RttStatus status;
    /**
     * If status is RTT_STATUS_FAIL_BUSY_TRY_LATER, this will be the time provided by the responder
     * as to when the request can be tried again. Applies to 2-sided RTT only. In sec, 1-31 sec.
     */
    byte retryAfterDuration;
    /**
     * RTT type.
     */
    RttType type;
    /**
     * Average rssi in 0.5 dB steps (e.g. 143 implies -71.5 dB).
     */
    int rssi;
    /**
     * Rssi spread in 0.5 dB steps (e.g. 5 implies 2.5 dB spread) - optional.
     */
    int rssiSpread;
    /**
     * 1-sided RTT: TX rate of RTT frame.
     * 2-sided RTT: TX rate of initiator's Ack in response to FTM frame.
     */
    WifiRateInfo txRate;
    /**
     * 1-sided RTT: TX rate of Ack from other side.
     * 2-sided RTT: TX rate of FTM frame coming from responder.
     */
    WifiRateInfo rxRate;
    /**
     * Round trip time in picoseconds.
     */
    long rtt;
    /**
     * Rtt standard deviation in picoseconds.
     */
    long rttSd;
    /**
     * Difference between max and min rtt times recorded in picoseconds.
     */
    long rttSpread;
    /**
     * Distance in mm (optional).
     */
    int distanceInMm;
    /**
     * Standard deviation in mm.
     */
    int distanceSdInMm;
    /**
     * Difference between max and min distance recorded in mm (optional).
     *
     * Note: Only applicable for IEEE 802.11mc
     */
    int distanceSpreadInMm;
    /**
     * Time of the measurement (in microseconds since boot).
     */
    long timeStampInUs;
    /**
     * Actual time taken by the FW to finish one burst measurement (in ms). Applies to 1-sided
     * and 2-sided IEEE 802.11mc RTT.
     */
    int burstDurationInMs;
    /**
     * Number of bursts allowed by the responder. Applies to 2-sided IEEE 802.11mc RTT only.
     */
    int negotiatedBurstNum;
    /**
     * For IEEE 802.11mc and IEEE 802.11az only.
     */
    WifiInformationElement lci;
    /**
     * For IEEE 802.11mc and IEEE 802.11az only.
     */
    WifiInformationElement lcr;
    /**
     * RTT channel frequency in MHz
     * If frequency is unknown, this will be set to 0.
     */
    int channelFreqMHz;
    /**
     * RTT packet bandwidth.
     * This value is an average bandwidth of the bandwidths of measurement frames. Cap the average
     * close to a specific valid RttBw.
     */
    RttBw packetBw;
    /**
     * IEEE 802.11az Transmit LTF repetitions used to get this result.
     */
    int txLtfRepetitionCount;
    /**
     * Minimum non-trigger based (non-TB) dynamic measurement time in milliseconds assigned by the
     * IEEE 802.11az responder.
     *
     * After initial non-TB negotiation, if the next ranging request for this peer comes in between
     * [ntbMinMeasurementTime, ntbMaxMeasurementTime], vendor software shall do the NDPA sounding
     * sequence for dynamic non-TB measurement.
     *
     * If the ranging request for this peer comes sooner than minimum measurement time, vendor
     * software shall return the cached result of the last measurement including the time stamp
     * |RttResult.timestamp|.
     */
    int ntbMinMeasurementTimeMillis;
    /**
     * Maximum non-trigger based (non-TB) dynamic measurement time in milliseconds assigned by the
     * IEEE 802.11az responder.
     *
     * After initial non-TB negotiation, if the next ranging request for this peer comes in between
     * [ntbMinMeasurementTime, ntbMaxMeasurementTime], vendor software shall do the NDPA sounding
     * sequence for dynamic non-TB measurement.
     *
     * If the ranging request for this peer comes later than the maximum measurement time, vendor
     * software shall clean up any existing IEEE 802.11ax non-TB ranging session and re-do the
     * non-TB ranging negotiation.
     */
    int ntbMaxMeasurementTimeMillis;
}
