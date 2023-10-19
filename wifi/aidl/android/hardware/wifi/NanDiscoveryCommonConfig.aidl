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

import android.hardware.wifi.NanDataPathSecurityConfig;
import android.hardware.wifi.NanMatchAlg;

/**
 * Configurations of NAN discovery sessions. Common to publish and subscribe discovery.
 */
@VintfStability
parcelable NanDiscoveryCommonConfig {
    /**
     * The ID of the discovery session being configured. A value of 0 specifies a request to create
     * a new discovery session. The new discovery session ID is returned with
     * |IWifiNanIfaceEventCallback.notifyStartPublishResponse| or
     * |IWifiNanIfaceEventCallback.notifyStartSubscribeResponse|.
     * NAN Spec: Service Descriptor Attribute (SDA) / Instance ID
     */
    byte sessionId;
    /**
     * The lifetime of the discovery session in seconds. A value of 0 means run forever or until
     * canceled using |IWifiIface.stopPublishRequest| or |IWifiIface.stopSubscribeRequest|.
     */
    char ttlSec;
    /**
     * Indicates the interval between two Discovery Windows in which the device supporting the
     * service is awake to transmit or receive the Service Discovery frames. Valid values of Awake
     * DW Interval are: 1, 2, 4, 8 and 16. A value of 0 will default to 1. Does not override
     * |NanBandSpecificConfig.discoveryWindowIntervalVal| configurations if those are specified.
     */
    char discoveryWindowPeriod;
    /**
     * The lifetime of the discovery session in number of transmitted SDF discovery packets. A value
     * of 0 means forever or until canceled using |IWifiIface.stopPublishRequest| or
     * |IWifiIface.stopSubscribeRequest|.
     */
    byte discoveryCount;
    /**
     * UTF-8 encoded string identifying the service.
     * Max length: |NanCapabilities.maxServiceNameLen|.
     * NAN Spec: The only acceptable single-byte UTF-8 symbols for a Service Name are alphanumeric
     * values (A-Z, a-z, 0-9), the hyphen ('-'), and the period ('.'). All valid multi-byte UTF-8
     * characters are acceptable in a Service Name.
     */
    byte[] serviceName;
    /**
     * Specifies how often to trigger |IWifiNanIfaceEventCallback.eventMatch| when continuously
     * discovering the same discovery session (with no changes).
     */
    NanMatchAlg discoveryMatchIndicator;
    /**
     * Arbitrary information communicated in discovery packets - there is no semantic meaning to
     * these bytes. They are passed-through from publisher to subscriber as-is with no parsing. Max
     * length: |NanCapabilities.maxServiceSpecificInfoLen|. NAN Spec: Service Descriptor Attribute
     * (SDA) / Service Info
     */
    byte[] serviceSpecificInfo;
    /**
     * Arbitrary information communicated in discovery packets - there is no semantic meaning to
     * these bytes. They are passed-through from publisher to subscriber as-is with no parsing. Max
     * length: |NanCapabilities.maxExtendedServiceSpecificInfoLen|. This info is using Generic
     * Service Protocol with setting Service Info type to 2 (Generic). NAN Spec: Service
     * Descriptor Extension Attribute (SDEA) / Service Info
     */
    byte[] extendedServiceSpecificInfo;
    /**
     * Ordered sequence of <length, value> pairs (|length| uses 1 byte and contains the number of
     * bytes in the |value| field) which specify further match criteria (beyond the service name).
     * The match behavior is specified in details in the NAN spec.
     * Publisher: used in SOLICITED or SOLICITED_UNSOLICITED sessions.
     * Subscriber: used in ACTIVE or PASSIVE sessions.
     * Max length: |NanCapabilities.maxMatchFilterLen|.
     * NAN Spec: matching_filter_rx
     */
    byte[] rxMatchFilter;
    /**
     * Ordered sequence of <length, value> pairs (|length| uses 1 byte and contains the number of
     * bytes in the |value| field) which specify further match criteria (beyond the service name).
     * The match behavior is specified in details in the NAN spec.
     * Publisher: used if provided.
     * Subscriber: used (if provided) only in ACTIVE sessions.
     * Max length: |NanCapabilities.maxMatchFilterLen|.
     * NAN Spec: matching_filter_tx and Service Descriptor Attribute (SDA) / Matching Filter
     */
    byte[] txMatchFilter;
    /**
     * Specifies whether or not the discovery session uses the
     * |NanBandSpecificConfig.rssiCloseProximity| value (configured in enable/configure requests) to
     * filter out matched discovered peers.
     * NAN Spec: Service Descriptor Attribute / Service Control / Discovery Range Limited.
     */
    boolean useRssiThreshold;
    /**
     * Controls whether or not the |IWifiNanIfaceEventCallback.eventPublishTerminated| (for publish
     * discovery sessions) or |IWifiNanIfaceEventCallback.eventSubscribeTerminated| (for subscribe
     * discovery sessions) will be delivered.
     */
    boolean disableDiscoveryTerminationIndication;
    /**
     * Controls whether or not |IWifiNanIfaceEventCallback.eventMatchExpired| will be delivered.
     */
    boolean disableMatchExpirationIndication;
    /**
     * Controls whether or not |IWifiNanIfaceEventCallback.eventFollowupReceived| will be delivered.
     */
    boolean disableFollowupReceivedIndication;
    /**
     * Security configuration of data-paths created in the context of this discovery session.
     * Security parameters can be overridden during the actual construction of the data-path -
     * allowing individual data-paths to have unique PMKs or passphrases.
     */
    NanDataPathSecurityConfig securityConfig;
    /**
     * Specifies whether or not there is a ranging requirement in this discovery session.
     * Ranging is only performed if all other match criteria with the peer are met. Ranging must
     * be performed if both peers in the discovery session (publisher and subscriber) set this
     * flag to true. Otherwise, if either peer sets this flag to false, ranging must not be
     * performed and must not impact discovery decisions. Note: Specifying that ranging is required
     * also implies that this device must automatically accept ranging requests from peers. NAN
     * Spec: Service Discovery Extension Attribute (SDEA) / Control / Ranging Require.
     */
    boolean rangingRequired;
    /**
     * Interval in ms between two ranging measurements. Only relevant if |rangingRequired| is true.
     * If the Awake DW interval specified either in |discoveryWindowPeriod| or in
     * |NanBandSpecificConfig.discoveryWindowIntervalVal| is larger than the ranging interval then
     * priority is given to Awake DW interval.
     */
    int rangingIntervalMs;
    /**
     * Bitmap of |NanRangingIndication| values indicating the type of ranging feedback
     * to be provided by discovery session matches in |IWifiNanIfaceEventCallback.eventMatch|.
     * Only relevant if |rangingRequired| is true.
     */
    int configRangingIndications;
    /**
     * The ingress and egress distance in cm. If ranging is enabled (|rangingEnabled| is true) then
     * |configRangingIndications| is used to determine whether ingress and/or egress (or neither)
     * are used to determine whether a match has occurred.
     * NAN Spec: Service Discovery Extension Attribute (SDEA) / Ingress & Egress Range Limit
     */
    char distanceIngressCm;
    char distanceEgressCm;
    /**
     * Specifies whether suspension can be possible in this discovery session.
     * The request would fail if |enableSessionSuspendability| is true but
     * |NanCapabilities.supportsSuspension| is false.
     */
    boolean enableSessionSuspendability;
}
