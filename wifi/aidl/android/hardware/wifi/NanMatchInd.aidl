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

import android.hardware.wifi.NanCipherSuiteType;
import android.hardware.wifi.NanIdentityResolutionAttribute;
import android.hardware.wifi.NanPairingConfig;
import android.hardware.wifi.common.OuiKeyedData;

/**
 * Match indication structure.
 */
@VintfStability
parcelable NanMatchInd {
    /**
     * Publish or subscribe discovery session ID of an existing discovery session.
     * NAN Spec: Service Descriptor Attribute (SDA) / Instance ID
     */
    byte discoverySessionId;
    /**
     * A unique ID of the peer. Can be subsequently used in |IWifiNanIface.transmitFollowupRequest|
     * or to set up a data-path.
     */
    int peerId;
    /**
     * The NAN Discovery (management) MAC address of the peer.
     */
    byte[6] addr;
    /**
     * The arbitrary information contained in the |NanDiscoveryCommonConfig.serviceSpecificInfo| of
     * the peer's discovery session configuration.
     * Max length: |NanCapabilities.maxServiceSpecificInfoLen|.
     * NAN Spec: Service Descriptor Attribute (SDA) / Service Info
     */
    byte[] serviceSpecificInfo;
    /**
     * Arbitrary information communicated in discovery packets. There is no semantic meaning to
     * these bytes. They are passed-through from publisher to subscriber as-is with no parsing. Max
     * length: |NanCapabilities.maxExtendedServiceSpecificInfoLen|. Spec: Service Descriptor
     * Extension Attribute (SDEA) / Service Info
     */
    byte[] extendedServiceSpecificInfo;
    /**
     * The match filter from the discovery packet (publish or subscribe) which caused service
     * discovery. Matches the |NanDiscoveryCommonConfig.txMatchFilter| of the peer's Unsolicited
     * publish message or of the local device's Active subscribe message.
     * Max length: |NanCapabilities.maxMatchFilterLen|.
     * NAN Spec: Service Descriptor Attribute (SDA) / Matching Filter
     */
    byte[] matchFilter;
    /**
     * Indicates the type of discovery: true if match occurred on a Beacon frame, false if the match
     * occurred on a Service Discovery Frame (SDF).
     */
    boolean matchOccurredInBeaconFlag;
    /**
     * Flag to indicate firmware is out of resources and that it can no longer track this Service
     * Name. Indicates that while |IWifiNanIfaceEventCallback.eventMatch| will be received, the
     * |NanDiscoveryCommonConfig.discoveryMatchIndicator| configuration will not be honored.
     */
    boolean outOfResourceFlag;
    /**
     * If RSSI filtering was enabled using |NanDiscoveryCommonConfig.useRssiThreshold| in discovery
     * session setup, then this field contains the received RSSI value. It will contain 0 if RSSI
     * filtering was not enabled.
     * RSSI values are returned without sign, e.g. -70dBm will be returned as 70.
     */
    byte rssiValue;
    /**
     * One of |NanCipherSuiteType| indicating the cipher type for data-paths constructed
     * in the context of this discovery session.
     * Valid if |peerRequiresSecurityEnabledInNdp| is true.
     */
    NanCipherSuiteType peerCipherType;
    /**
     * Indicates whether or not the peer requires security enabled in any data-path (NDP)
     * constructed in the context of this discovery session. The |cipherType| specifies the cipher
     * type for such data-paths. NAN Spec: Service Discovery Extension Attribute (SDEA) / Control /
     * Security Required
     */
    boolean peerRequiresSecurityEnabledInNdp;
    /**
     * Indicates whether or not the peer requires (and hence allows) ranging in the context of this
     * discovery session.
     * Note that ranging is only performed if all other match criteria with the peer are met.
     * NAN Spec: Service Discovery Extension Attribute (SDEA) / Control / Ranging Require.
     */
    boolean peerRequiresRanging;
    /**
     * Ranging indication supersedes the NanMatchAlg specification.
     * Ex: If NanMatchAlg is MATCH_ONCE, but ranging indication is continuous, then continuous
     * match notifications will be received (with ranging information).
     * Ranging indication data is provided if Ranging required is enabled in the discovery
     * specification and:
     *   1) continuous ranging is specified.
     *   2) ingress/egress is specified and:
     *       - notify once for ingress >= ingress_distance and egress <= egress_distance,
     *       - same for ingress_egress_both
     * If the Awake DW intervals are larger than the ranging intervals, then priority is given
     * to the device DW intervals.
     *
     * If ranging was required and executed, this contains the distance to the peer in mm. The
     * |rangingIndicationType| field specifies the event which triggered ranging.
     */
    int rangingMeasurementInMm;
    /**
     * Bitmap of |NanRangingIndication| values indicating the ranging event(s) which triggered the
     * ranging. e.g. can indicate that continuous ranging was requested, or else that an ingress
     * event occurred.
     */
    int rangingIndicationType;
    /**
     * Security Context Identifier attribute contains PMKID. Shall be included in NDP setup and
     * response messages. Security Context Identifier identifies the Security Context. For NAN
     * Shared Key Cipher Suite, this field contains the 16 octet PMKID identifying the PMK used for
     * setting up the Secure Data Path.
     */
    byte[] scid;
    /**
     * The config for NAN pairing set by the peer
     */
    NanPairingConfig peerPairingConfig;
    /**
     * The NIRA from peer for NAN pairing verification
     */
    NanIdentityResolutionAttribute peerNira;
    /**
     * Optional vendor-specific parameters. Null value indicates
     * that no vendor data is provided.
     */
    @nullable OuiKeyedData[] vendorData;
}
