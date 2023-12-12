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
import android.hardware.wifi.RttPeerType;
import android.hardware.wifi.RttPreamble;
import android.hardware.wifi.RttType;
import android.hardware.wifi.WifiChannelInfo;

/**
 * RTT configuration.
 */
@VintfStability
parcelable RttConfig {
    /**
     * Peer device mac address.
     */
    byte[6] addr;
    /**
     * 1-sided or 2-sided RTT (IEEE 802.11mc or IEEE 802. 11az).
     */
    RttType type;
    /**
     * Optional peer device hint (STA, P2P, AP).
     */
    RttPeerType peer;
    /**
     * Required for STA-AP mode, optional for P2P, NBD etc.
     */
    WifiChannelInfo channel;
    /**
     * Time interval between bursts (units: 100 ms).
     * Applies to 1-sided and 2-sided RTT multi-burst requests.
     * Range: 0-31, 0: no preference by initiator (2-sided RTT).
     *
     * Note: Applicable to IEEE 802.11mc only.
     */
    int burstPeriod;
    /**
     * Total number of RTT bursts to be executed. Will be
     * specified in the same way as the parameter "Number of
     * Burst Exponent" found in the FTM frame format. This
     * applies to both 1-sided RTT and 2-sided RTT. Valid
     * values are 0 to 15 as defined in 802.11mc std.
     * 0 means single shot.
     * The implication of this parameter on the maximum
     * number of RTT results is the following:
     * for 1-sided RTT: max num of RTT results = (2^num_burst)*(num_frames_per_burst)
     * for 2-sided RTT: max num of RTT results = (2^num_burst)*(num_frames_per_burst - 1)
     *
     * Note: Applicable to IEEE 802.11mc only. For IEEE 802.11az refer
     * |RttConfig.txLtfRepetitionCount|.
     */
    int numBurst;
    /**
     * Number of frames per burst.
     * Minimum value = 1, Maximum value = 31
     * For 2-sided, this equals the number of FTM frames
     * to be attempted in a single burst. This also
     * equals the number of FTM frames that the
     * initiator will request that the responder sends
     * in a single frame.
     *
     * Note: Applicable to IEEE 802.11mc only.
     */
    int numFramesPerBurst;
    /**
     * Number of retries for a failed RTT frame.
     * Applies to 1-sided RTT only. Minimum value = 0, Maximum value = 3
     */
    int numRetriesPerRttFrame;
    /**
     * The following fields are only valid for 2-side RTT.
     *
     *
     * Maximum number of retries that the initiator can
     * retry an FTMR frame.
     * Minimum value = 0, Maximum value = 3
     */
    int numRetriesPerFtmr;
    /**
     * Whether to request location civic info or not.
     */
    boolean mustRequestLci;
    /**
     * Whether to request location civic records or not.
     */
    boolean mustRequestLcr;
    /**
     * Applies to 1-sided and 2-sided IEEE 802.11mc RTT. Valid values will
     * be 2-11 and 15 as specified by the IEEE 802.11mc std for
     * the FTM parameter burst duration. In a multi-burst
     * request, if responder overrides with larger value,
     * the initiator will return failure. In a single-burst
     * request, if responder overrides with larger value,
     * the initiator will send TMR_STOP to terminate RTT
     * at the end of the burst_duration it requested.
     */
    int burstDuration;
    /**
     * RTT preamble to be used in the RTT frames.
     */
    RttPreamble preamble;
    /**
     * RTT BW to be used in the RTT frames.
     */
    RttBw bw;
    /**
     * IEEE 802.11az Non-Trigger-based (non-TB) minimum measurement time in milliseconds.
     */
    int ntbMinMeasurementTimeMillis;
    /**
     * IEEE 802.11az Non-Trigger-based (non-TB) maximum measurement time in milliseconds.
     */
    int ntbMaxMeasurementTimeMillis;
    /**
     * Multiple transmissions of HE-LTF symbols in an HE Ranging NDP. A value of 1 indicates no
     * repetition.
     */
    int txLtfRepetitionCount;
}
