/*
 * Copyright 2021 The Android Open Source Project
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

package android.hardware.tv.tuner;

import android.hardware.tv.tuner.DemuxTlvFilterSettingsFilterSettings;

/**
 * Filter Settings for a TLV filter.
 * @hide
 */
@VintfStability
parcelable DemuxTlvFilterSettings {
    /**
     * Packet type according to ITU-R BT.1869.
     * 0x01: IPv4 packet
     * 0x02: IPv6 packet
     * 0x03: IP packet with header compression
     * 0xFE: Signaling packet
     * 0xFF: NULL packet
     */
    int packetType;

    /**
     * true if the filtered data is commpressed ip packet
     */
    boolean isCompressedIpPacket;

    DemuxTlvFilterSettingsFilterSettings filterSettings;
}
