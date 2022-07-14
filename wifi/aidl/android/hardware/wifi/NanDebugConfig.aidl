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

/**
 * Debug configuration parameters. Many of these allow non-standard-compliant operation and are
 * not intended for normal operational mode.
 */
@VintfStability
parcelable NanDebugConfig {
    /**
     * Specification of the lower 2 bytes of the cluster ID. The cluster ID is 50-60-9a-01-00-00 to
     * 50-60-9a-01-FF-FF. Configuration is of the bottom and top values of the range (which default
     * to 0x0000 and 0xFFFF respectively).
     * Configuration is only used if |validClusterIdVals| is set to true.
     */
    boolean validClusterIdVals;
    char clusterIdBottomRangeVal;
    char clusterIdTopRangeVal;
    /**
     * NAN management interface address. If specified (|validIntfAddrVal| is true), then overrides
     * any other configuration (specifically the default randomization configured by
     * |NanConfigRequest.macAddressRandomizationIntervalSec|).
     */
    boolean validIntfAddrVal;
    byte[6] intfAddrVal;
    /**
     * Combination of the 24 bit Organizationally Unique ID (OUI) and the 8 bit OUI type.
     * Used if |validOuiVal| is set to true.
     */
    boolean validOuiVal;
    int ouiVal;
    /**
     * Force the Random Factor to the specified value for all transmitted Sync/Discovery beacons.
     * Used if |validRandomFactorForceVal| is set to true.
     * NAN Spec: Master Indication Attribute / Random Factor
     */
    boolean validRandomFactorForceVal;
    byte randomFactorForceVal;
    /**
     * Forces the hop-count for all transmitted Sync and Discovery Beacons NO matter the real
     * hop-count being received over the air. Used if the |validHopCountForceVal| flag is set to
     * true.
     * NAN Spec: Cluster Attribute / Anchor Master Information / Hop Count to Anchor Master
     */
    boolean validHopCountForceVal;
    byte hopCountForceVal;
    /**
     * Frequency in MHz to of the discovery channel in the specified band. Indexed by
     * |NanBandIndex|. Used if the |validDiscoveryChannelVal| is set to true.
     */
    boolean validDiscoveryChannelVal;
    int[3] discoveryChannelMhzVal;
    /**
     * Specifies whether sync/discovery beacons are transmitted in the specified band. Indexed by
     * |NanBandIndex|. Used if the |validUseBeaconsInBandVal| is set to true.
     */
    boolean validUseBeaconsInBandVal;
    boolean[3] useBeaconsInBandVal;
    /**
     * Specifies whether SDF (service discovery frames) are transmitted in the specified band.
     * Indexed by |NanBandIndex|. Used if the |validUseSdfInBandVal| is set to true.
     */
    boolean validUseSdfInBandVal;
    boolean[3] useSdfInBandVal;
}
