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

package android.hardware.radio.data;

import android.hardware.radio.data.ApnAuthType;
import android.hardware.radio.data.PdpProtocolType;
import android.hardware.radio.data.TrafficDescriptor;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable DataProfileInfo {
    const int ID_DEFAULT = 0;
    const int ID_TETHERED = 1;
    const int ID_IMS = 2;
    const int ID_FOTA = 3;
    const int ID_CBS = 4;
    /**
     * Start of OEM-specific profiles
     */
    const int ID_OEM_BASE = 1000;
    const int ID_INVALID = 0xFFFFFFFF;

    const int TYPE_COMMON = 0;
    const int TYPE_3GPP = 1;
    const int TYPE_3GPP2 = 2;

    /**
     * Innfrastructure type unknown. This is only for initializing.
     */
    const int INFRASTRUCTURE_UNKNOWN = 0;

    /**
     * Indicating this APN can be used when the device is using terrestrial cellular networks.
     */
    const int INFRASTRUCTURE_CELLULAR = 1 << 0;

    /**
     * Indicating this APN can be used when the device is attached to satellite.
     */
    const int INFRASTRUCTURE_SATELLITE = 1 << 1;

    /**
     * ID of the data profile.
     * Values are ID_
     */
    int profileId;
    /**
     * The APN name.
     */
    String apn;
    /**
     * PDP_type values.
     */
    PdpProtocolType protocol;
    /**
     * PDP_type values used on roaming network.
     */
    PdpProtocolType roamingProtocol;
    /**
     * APN authentication type.
     */
    ApnAuthType authType;
    /**
     * The username for APN, or empty string.
     */
    String user;
    /**
     * The password for APN, or empty string.
     */
    String password;
    /**
     * Data profile technology type.
     * Values are TYPE_
     */
    int type;
    /**
     * The period in seconds to limit the maximum connections.
     */
    int maxConnsTime;
    /**
     * The maximum connections during maxConnsTime.
     */
    int maxConns;
    /**
     * The required wait time in seconds after a successful UE initiated disconnect of a given PDN
     * connection before the device can send a new PDN connection request for that given PDN.
     */
    int waitTime;
    /**
     * True to enable the profile, false to disable.
     */
    boolean enabled;
    /**
     * Supported APN types bitmap. See ApnTypes for the value of each bit.
     */
    int supportedApnTypesBitmap;
    /**
     * The bearer bitmap. See RadioAccessFamily for the value of each bit.
     */
    int bearerBitmap;
    /**
     * Maximum transmission unit (MTU) size in bytes for IPv4.
     */
    int mtuV4;
    /**
     * Maximum transmission unit (MTU) size in bytes for IPv6.
     */
    int mtuV6;
    /**
     * True if this data profile was used to bring up the last default (i.e internet) data
     * connection successfully.
     */
    boolean preferred;
    /**
     * If true, modem must persist this data profile and profileId must not be set to ID_INVALID.
     * If the same data profile exists, this data profile must overwrite it.
     */
    boolean persistent;
    /**
     * Indicates the PDU session brought up by this data profile should be always-on.
     * An always-on PDU Session is a PDU Session for which User Plane resources have to be
     * activated during every transition from CM-IDLE mode to CM-CONNECTED state.
     * See 3GPP TS 23.501 section 5.6.13 for the details.
     */
    boolean alwaysOn;
    /**
     * TrafficDescriptor for which data connection needs to be established.
     * It is used for URSP traffic matching as described in TS 24.526 Section 4.2.2.
     * It includes an optional DNN which, if present, must be used for traffic matching --
     * it does not specify the end point to be used for the data call. The end point is specified by
     * apn; apn must be used as the end point if one is not specified through URSP rules.
     */
    TrafficDescriptor trafficDescriptor;
    /**
     * The infrastructure bitmap which the APN can be used on. For example, some APNs can only
     * be used when the device is using cellular network, using satellite network, or can be used
     * in either cases.
     */
    int infrastructureBitmap = INFRASTRUCTURE_UNKNOWN;
}
