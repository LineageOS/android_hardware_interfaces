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

import android.hardware.wifi.common.OuiKeyedData;

/**
 * Parameters used for |ISupplicantP2pIfaceCallback.onInvitationReceivedWithParams|
 */
@VintfStability
parcelable P2pInvitationEventParams {
    /**
     * MAC address of the device that sent the invitation.
     */
    byte[6] srcAddress;

    /**
     * P2P device MAC Address of the group owner.
     */
    byte[6] goDeviceAddress;

    /**
     * BSSID of the group.
     */
    byte[6] bssid;

    /**
     * Persistent network ID of the group.
     */
    int persistentNetworkId;

    /**
     * Frequency on which the invitation was received.
     */
    int operatingFrequencyMHz;

    /**
     * Optional vendor-specific parameters. Null value indicates
     * that no vendor data is provided.
     */
    @nullable OuiKeyedData[] vendorData;
}
