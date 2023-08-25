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

package android.hardware.wifi.supplicant;

/**
 * Multi-Link Operation (MLO) Link IEEE Std 802.11-be.
 * The information for MLO link needed by 802.11be standard.
 */
@VintfStability
parcelable MloLink {
    /**
     * Link ID
     */
    byte linkId;
    /**
     * STA Link MAC Address
     */
    byte[/* 6 */] staLinkMacAddress;
    /**
     * Bitset where each bit indicates TID mapped to this link in uplink and
     * downlink direction.
     *
     * Traffic Identifier (TID) is an identifier used to classify a packet. It
     * is represented as a four bit number identifying the QoS traffic within
     * MAC data service. There are 16 possible values for TID, out of only 8 are
     * practically used to identify differentiated services.
     *
     * A TID-to-link mapping indicates links on which frames belonging to each
     * TID can be exchanged. IEEE 802.11be draft 2.0 defines the mapping for TID
     * values between 0 to 7 only. Once associated, an MLO link state is
     * considered as active if at least one TID is mapped to the link. Link
     * state is considered as idle if no TID is mapped to the link.
     *
     * TIDs can be mapped to uplink, downlink or both directions.
     * e.g.
     *  - TID 4 is mapped to this link in uplink direction, if bit 4 in
     *    MloLink#tids_uplink_map is set.
     *  - TID 2 is mapped to both directions for this link, if bit 2 of both
     *    MloLink#tids_uplink_map and MloLink#tids_downlink_map are set.
     *
     * In case of default link mapping, tids_uplink_map and tids_downlink_map
     * is set to 0xFF for all the links.
     *
     */
    byte tidsUplinkMap;
    byte tidsDownlinkMap;
    /**
     * AP Link MAC Address
     */
    @nullable byte[6] apLinkMacAddress;
    /**
     * Frequency on which the link operates in MHz.
     */
    int frequencyMHz;
}
