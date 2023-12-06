/*
 * Copyright 2023 The Android Open Source Project
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

/**
 * Each enum value represents a message type on the Non-Access Stratum (NAS). The relevant cellular
 * generation is noted for each message type. Sample spec references are provided, but generally
 * only reference one network generation's spec.
 *
 * @hide
 */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum NasProtocolMessage {
    UNKNOWN = 0,
    // Sample Reference: 3GPP TS 24.301 8.2.4
    // Applies to 2g, 3g, and 4g networks
    ATTACH_REQUEST = 1,
    // Sample Reference: 3GPP TS 24.301 8.2.19
    // Applies to 2g, 3g, 4g, and 5g networks
    IDENTITY_RESPONSE = 2,
    // Sample Reference: 3GPP TS 24.301 8.2.11
    // Applies to 2g, 3g, and 4g networks
    DETACH_REQUEST = 3,
    // Sample Reference: 3GPP TS 24.301 8.2.29
    // Note: that per the spec, only temporary IDs should be sent
    // in the TAU Request, but since the EPS Mobile Identity field
    // supports IMSIs, this is included as an extra safety measure
    // to combat implementation bugs.
    // Applies to 4g and 5g networks
    TRACKING_AREA_UPDATE_REQUEST = 4,
    // Sample Reference: 3GPP TS 24.008 4.4.3
    // Applies to 2g and 3g networks
    LOCATION_UPDATE_REQUEST = 5,
    // Reference: 3GPP TS 24.008 4.7.7.1
    // Applies to 2g and 3g networks
    AUTHENTICATION_AND_CIPHERING_RESPONSE = 6,
    // Reference: 3GPP TS 24.501 8.2.6
    // Applies to 5g networks
    REGISTRATION_REQUEST = 7,
    // Reference: 3GPP TS 24.501 8.2.12
    // Applies to 5g networks
    DEREGISTRATION_REQUEST = 8,
    // Reference: 3GPP TS 24.008 9.2.4
    // Applies to 2g and 3g networks
    CM_REESTABLISHMENT_REQUEST = 9,
    // Reference: 3GPP TS 24.008 9.2.9
    // Applies to 2g and 3g networks
    CM_SERVICE_REQUEST = 10,
    // Reference: 3GPP TS 24.008 9.2.14
    // Applies to 2g and 3g networks. Used for circuit-switched detach.
    IMSI_DETACH_INDICATION = 11
}
