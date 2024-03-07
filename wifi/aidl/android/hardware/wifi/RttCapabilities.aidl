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
import android.hardware.wifi.RttPreamble;

/**
 * RTT Capabilities.
 */
@VintfStability
parcelable RttCapabilities {
    /**
     * Whether 1-sided rtt data collection is supported.
     */
    boolean rttOneSidedSupported;
    /**
     * Whether ftm rtt data collection is supported.
     */
    boolean rttFtmSupported;
    /**
     * Whether initiator supports Location Configuration Information (LCI) request. Applies to
     * 2-sided RTT.
     */
    boolean lciSupported;
    /**
     * Whether initiator supports Location Civic Report (LCR) request. Applies to 2-sided RTT.
     */
    boolean lcrSupported;
    /**
     * Whether IEEE 802.11mc responder mode is supported.
     */
    boolean responderSupported;
    /**
     * Bit mask indicating what preamble is supported by IEEE 802.11mc initiator.
     * Combination of |RttPreamble| values.
     */
    RttPreamble preambleSupport;
    /**
     * Bit mask indicating what BW is supported by IEEE 802.11mc initiator.
     * Combination of |RttBw| values.
     */
    RttBw bwSupport;
    /**
     * Draft 11mc spec version supported by chip.
     * For instance, version 4.0 must be 40 and version 4.3 must be 43 etc.
     */
    byte mcVersion;
    /**
     * Bit mask indicating what preamble is supported by IEEE 802.11az initiator.
     * Combination of |RttPreamble| values.
     */
    RttPreamble azPreambleSupport;
    /**
     * Bit mask indicating what BW is supported by IEEE 802.11az initiator.
     * Combination of |RttBw| values.
     */
    RttBw azBwSupport;
    /**
     * Whether the initiator supports IEEE 802.11az Non-Trigger-based (non-TB) measurement.
     */
    boolean ntbInitiatorSupported;
    /**
     * Whether IEEE 802.11az Non-Trigger-based (non-TB) responder mode is supported.
     */
    boolean ntbResponderSupported;
    /**
     * Maximum HE LTF repetitions the IEEE 802.11az initiator is capable of transmitting in the
     * preamble of I2R NDP.
     */
    int maxTxLtfRepetitionCount;
}
