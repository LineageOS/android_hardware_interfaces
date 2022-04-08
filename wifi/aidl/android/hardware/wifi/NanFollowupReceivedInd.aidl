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
 * Follow up message received from peer indication structure.
 */
@VintfStability
parcelable NanFollowupReceivedInd {
    /**
     * Discovery session (publish or subscribe) ID of a previously created discovery session. The
     * message is received in the context of this discovery session.
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
     * Indicates whether received in a further availability window (FAW) if true, or in a discovery
     * window (DW) if false.
     */
    boolean receivedInFaw;
    /**
     * Received message from the peer. There is no semantic meaning to these bytes. They are
     * passed-through from sender to receiver as-is with no parsing.
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
}
