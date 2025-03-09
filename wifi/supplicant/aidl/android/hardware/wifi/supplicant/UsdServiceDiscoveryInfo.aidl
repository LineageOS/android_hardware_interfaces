/*
 * Copyright (C) 2024 The Android Open Source Project
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

import android.hardware.wifi.supplicant.UsdServiceProtoType;

/**
 * Information about a USD discovery session with a specific peer.
 */
@VintfStability
parcelable UsdServiceDiscoveryInfo {
    /**
     * Identifier for this device.
     */
    int ownId;

    /**
     * Identifier for the discovered peer device.
     */
    int peerId;

    /**
     * MAC address of the discovered peer device.
     */
    byte[6] peerMacAddress;

    /**
     * Match filter from the discovery packet (publish or subscribe) which caused service discovery.
     */
    byte[] matchFilter;

    /**
     * Service protocol that is being used (ex. Generic, CSA Matter).
     */
    UsdServiceProtoType protoType;

    /**
     * Arbitrary service specific information communicated in discovery packets.
     * There is no semantic meaning to these bytes. They are passed-through from publisher to
     * subscriber as-is with no parsing.
     */
    byte[] serviceSpecificInfo;

    /**
     * Whether Further Service Discovery (FSD) is enabled.
     */
    boolean isFsd;
}
