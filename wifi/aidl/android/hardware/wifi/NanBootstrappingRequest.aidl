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

import android.hardware.wifi.NanBootstrappingMethod;

/**
 * See Wi-Fi Aware R4.0 section 9.5.21.7
 */
@VintfStability
parcelable NanBootstrappingRequest {
    /**
     * ID of the peer. Obtained as part of an earlier |IWifiNanIfaceEventCallback.eventMatch| or
     * |IWifiNanIfaceEventCallback.eventFollowupReceived|.
     */
    int peerId;

    /**
     * NAN management interface MAC address of the peer. Obtained as part of an earlier
     * |IWifiNanIfaceEventCallback.eventMatch| or
     * |IWifiNanIfaceEventCallback.eventFollowupReceived|.
     */
    byte[6] peerDiscMacAddr;

    /**
     * One of |NanBootstrappingMethod| indicating the bootstrapping method in the request.
     */
    NanBootstrappingMethod requestBootstrappingMethod;

    /**
     * Cookie received from previous |NanBootstrappingConfirmInd| for comeback request.
     */
    byte[] cookie;

    /**
     * Identify if it is a request for come back response
     */
    boolean isComeback;

    /**
     * ID of an active publish or subscribe discovery session. Follow-up message is transmitted in
     * the context of the discovery session. NAN Spec: Service Descriptor Attribute (SDA) / Instance
     * ID
     */
    byte discoverySessionId;
}
