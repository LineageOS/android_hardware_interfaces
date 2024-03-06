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

import android.hardware.wifi.NanBandSpecificConfig;
import android.hardware.wifi.common.OuiKeyedData;

/**
 * Configuration parameters of NAN. Used when enabling and re-configuring a NAN cluster.
 */
@VintfStability
parcelable NanConfigRequest {
    /**
     * Master preference of this device.
     * NAN Spec: Master Indication Attribute / Master Preference
     */
    byte masterPref;
    /**
     * Controls whether or not the |IWifiNanIfaceEventCallback.eventClusterEvent| will be delivered
     * for |NanClusterEventType.DISCOVERY_MAC_ADDRESS_CHANGED|.
     */
    boolean disableDiscoveryAddressChangeIndication;
    /**
     * Controls whether or not the |IWifiNanIfaceEventCallback.eventClusterEvent| will be delivered
     * for |NanClusterEventType.STARTED_CLUSTER|.
     */
    boolean disableStartedClusterIndication;
    /**
     * Controls whether or not the |IWifiNanIfaceEventCallback.eventClusterEvent| will be delivered
     * for |NanClusterEventType.JOINED_CLUSTER|.
     */
    boolean disableJoinedClusterIndication;
    /**
     * Control whether publish service IDs are included in Sync/Discovery beacons.
     * NAN Spec: Service ID List Attribute
     */
    boolean includePublishServiceIdsInBeacon;
    /**
     * If |includePublishServiceIdsInBeacon| is true, then specifies the number of publish service
     * IDs to include in the Sync/Discovery beacons. Value = 0: include as many service IDs as will
     * fit into the maximum allowed beacon frame size. Value must fit within 7 bits - i.e. <= 127.
     */
    byte numberOfPublishServiceIdsInBeacon;
    /**
     * Control whether subscribe service IDs are included in Sync/Discovery beacons.
     * Spec: Subscribe Service ID List Attribute
     */
    boolean includeSubscribeServiceIdsInBeacon;
    /**
     * If |includeSubscribeServiceIdsInBeacon| is true, then specifies the number of subscribe
     * service IDs to include in the Sync/Discovery beacons. Value = 0: include as many service IDs
     * as will fit into the maximum allowed beacon frame size. Value must fit within 7 bits - i.e.
     * <= 127.
     */
    byte numberOfSubscribeServiceIdsInBeacon;
    /**
     * Number of samples used to calculate RSSI.
     */
    char rssiWindowSize;
    /**
     * Specifies the interval in seconds that the NAN management interface MAC address is
     * randomized. A value of 0 is used to disable the MAC address randomization.
     */
    int macAddressRandomizationIntervalSec;
    /**
     * Additional configuration provided per band. Indexed by |NanBandIndex|.
     */
    NanBandSpecificConfig[3] bandSpecificConfig;
    /**
     * Optional vendor-specific parameters. Null value indicates
     * that no vendor data is provided.
     */
    @nullable OuiKeyedData[] vendorData;
}
