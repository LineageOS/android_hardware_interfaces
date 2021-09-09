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

package android.hardware.radio;

import android.hardware.radio.DataCallFailCause;
import android.hardware.radio.DataConnActiveStatus;
import android.hardware.radio.HandoverFailureMode;
import android.hardware.radio.LinkAddress;
import android.hardware.radio.OptionalSliceInfo;
import android.hardware.radio.PdpProtocolType;
import android.hardware.radio.Qos;
import android.hardware.radio.QosSession;
import android.hardware.radio.TrafficDescriptor;

@VintfStability
parcelable SetupDataCallResult {
    /**
     * Data call fail cause. DataCallFailCause.NONE if no error.
     */
    DataCallFailCause cause;
    /**
     * If cause is not DataCallFailCause.NONE, this field indicates the network suggested data
     * retry back-off time in milliseconds. Negative value indicates network does not give any
     * suggestion. 0 indicates retry should be performed immediately. 0x7fffffffffffffff indicates
     * the device should not retry data setup anymore. During this time, no calls to
     * IRadio.setupDataCall for this APN will be made unless IRadioIndication.unthrottleApn is sent
     * with the same APN.
     */
    long suggestedRetryTime;
    /**
     * Context ID, uniquely identifies this data connection.
     */
    int cid;
    /**
     * Data connection active status.
     */
    DataConnActiveStatus active;
    /**
     * PDP protocol type. If cause is DataCallFailCause.ONLY_SINGLE_BEARER_ALLOWED, this is the
     * protocol type supported, such as "IP" or "IPV6".
     */
    PdpProtocolType type;
    /**
     * The network interface name.
     */
    String ifname;
    /**
     * List of link address.
     */
    LinkAddress[] addresses;
    /**
     * List of DNS server addresses, e.g., "192.0.1.3" or "192.0.1.11 2001:db8::1". Empty if no dns
     * server addresses returned.
     */
    String[] dnses;
    /**
     * List of default gateway addresses, e.g., "192.0.1.3" or "192.0.1.11 2001:db8::1".
     * When empty, the addresses represent point to point connections.
     */
    String[] gateways;
    /**
     * List of P-CSCF (Proxy Call State Control Function) addresses via PCO (Protocol Configuration
     * Option), e.g., "2001:db8::1 2001:db8::2 2001:db8::3". Empty if not IMS client.
     */
    String[] pcscf;
    /**
     * MTU received from network for IPv4.
     * Value <= 0 means network has either not sent a value or sent an invalid value.
     */
    int mtuV4;
    /**
     * MTU received from network for IPv6.
     * Value <= 0 means network has either not sent a value or sent an invalid value.
     */
    int mtuV6;
    /**
     * Default bearer QoS. Applicable to LTE and NR
     */
    Qos defaultQos;
    /**
     * Active QOS sessions of the dedicated bearers. Applicable to PDNs that support dedicated
     * bearers.
     */
    QosSession[] qosSessions;
    /**
     * Specifies the fallback mode on an IWLAN handover failure.
     */
    HandoverFailureMode handoverFailureMode;
    /**
     * The allocated pdu session id for this data call. A value of 0 means no pdu session id was
     * attached to this call. Reference: 3GPP TS 24.007 section 11.2.3.1b.
     */
    int pduSessionId;
    /**
     * Slice used for this data call. It is valid only when this data call is on AccessNetwork:NGRAN
     */
    OptionalSliceInfo sliceInfo;
    /**
     * TrafficDescriptors for which this data call must be used. It only includes the TDs for which
     * a data call has been requested so far; it is not an exhaustive list.
     */
    TrafficDescriptor[] trafficDescriptors;
}
