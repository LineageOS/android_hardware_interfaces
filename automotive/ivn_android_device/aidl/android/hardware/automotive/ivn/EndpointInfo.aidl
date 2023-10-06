/*
 * Copyright (C) 2023 The Android Open Source Project
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

package android.hardware.automotive.ivn;

import android.hardware.automotive.ivn.ConnectProtocol;
import android.hardware.automotive.ivn.HardwareIdentifiers;

/**
 * Network endpoint information for an Android instance running on a difference device.
 *
 * The device is in the same vehicle as this device.
 */
@JavaDerive(equals=true, toString=true)
@VintfStability
parcelable EndpointInfo {
    /**
     * The connection protocol. Only supports TCP/IP for now.
     */
    ConnectProtocol connectProtocol;
    /**
     * The IP address.
     */
    String ipAddress;
    /**
     * The port number exposed for connecting.
     */
    int portNumber;
    /**
     * Hardware identifiers.
     *
     * The hardware identifiers for the endpoint as defined in [Attestation Hardware Identifiers]
     * {@link
     * https://source.android.com/docs/security/features/keystore/attestation#hardware-identifiers}
     */
    HardwareIdentifiers hardwareId;
}
