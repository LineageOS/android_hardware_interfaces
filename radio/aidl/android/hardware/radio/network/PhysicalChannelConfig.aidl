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

package android.hardware.radio.network;

import android.hardware.radio.RadioTechnology;
import android.hardware.radio.network.CellConnectionStatus;
import android.hardware.radio.network.PhysicalChannelConfigBand;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable PhysicalChannelConfig {
    /**
     * Connection status for cell. Valid values are PRIMARY_SERVING and SECONDARY_SERVING
     */
    CellConnectionStatus status;
    /**
     * The radio technology for this physical channel
     */
    RadioTechnology rat;
    /**
     * Downlink Absolute Radio Frequency Channel Number
     */
    int downlinkChannelNumber;
    /**
     * Uplink Absolute Radio Frequency Channel Number
     */
    int uplinkChannelNumber;
    /**
     * Downlink cell bandwidth, in kHz
     */
    int cellBandwidthDownlinkKhz;
    /**
     * Uplink cell bandwidth, in kHz
     */
    int cellBandwidthUplinkKhz;
    /**
     * A list of data calls mapped to this physical channel. The context id must match the cid of
     * SetupDataCallResult. An empty list means the physical channel has no data call mapped to it.
     */
    int[] contextIds;
    /**
     * The physical cell identifier for this cell.
     * In UTRAN, this value is primary scrambling code. The range is [0, 511].
     * Reference: 3GPP TS 25.213 section 5.2.2.
     * In EUTRAN, this value is physical layer cell identity. The range is [0, 503].
     * Reference: 3GPP TS 36.211 section 6.11.
     * In NGRAN, this value is physical layer cell identity. The range is [0, 1007].
     * Reference: 3GPP TS 38.211 section 7.4.2.1.
     */
    int physicalCellId;
    /**
     * The frequency band to scan.
     */
    PhysicalChannelConfigBand band;
}
