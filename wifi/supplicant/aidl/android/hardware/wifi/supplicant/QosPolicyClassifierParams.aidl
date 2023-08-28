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

import android.hardware.wifi.supplicant.IpVersion;
import android.hardware.wifi.supplicant.PortRange;
import android.hardware.wifi.supplicant.ProtocolNextHeader;
import android.hardware.wifi.supplicant.QosPolicyClassifierParamsMask;

/**
 * QoS policy classifier parameters. Refer section 5.4 of the
 * WFA (WiFi Alliance) QoS Management Specification v2.0.
 */
@VintfStability
parcelable QosPolicyClassifierParams {
    IpVersion ipVersion;

    /**
     * Classifier bit mask to identify filled fields. Setting a bit
     * in the mask to 1 means the corresponding field in this struct
     * has a value. Otherwise, that field should be ignored.
     */
    QosPolicyClassifierParamsMask classifierParamMask;

    /** Source IP address. */
    byte[] srcIp;

    /** Destination IP address. */
    byte[] dstIp;

    /** Source port. */
    int srcPort;

    /**
     * Destination port range. In the case of a single destination port,
     * both startPort and endPort will have the same values.
     */
    PortRange dstPortRange;

    /** Represents protocol for IPv4 and Next Header for IPv6. */
    ProtocolNextHeader protocolNextHdr;

    /** Applicable only for IPv6. */
    byte[/* 3 */] flowLabelIpv6;

    /**
     * Domain Name encoded and formatted in accordance with the rules for
     * "reg-name" in RFC 3986.
     */
    String domainName;

    /**
     * Differentiated Services Code Point (DSCP) value.
     * Used by AP for mapping the data streams to apply the user priority.
     */
    byte dscp;
}
