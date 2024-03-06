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

package android.hardware.wifi.supplicant;

import android.hardware.wifi.common.OuiKeyedData;

/**
 * Parameters passed as a part of a P2P Device found event.
 */
@VintfStability
parcelable P2pDeviceFoundEventParams {
    /**
     * MAC address of the device found. This must either be the P2P device address
     * for a peer which is not in a group, or the P2P interface address for
     * a peer which is a Group Owner.
     */
    byte[6] srcAddress;

    /**
     * P2P device address.
     */
    byte[6] p2pDeviceAddress;

    /**
     * Type of device. Refer to section B.1 of the Wifi P2P Technical
     * specification v1.2.
     */
    byte[] primaryDeviceType;

    /**
     * Name of the device.
     */
    String deviceName;

    /**
     * Mask of |WpsConfigMethods| indicating the WPS configuration methods
     * supported by the device.
     */
    int configMethods;

    /**
     * Refer to section 4.1.4 of the Wifi P2P Technical specification v1.2.
     */
    byte deviceCapabilities;

    /**
     * Mask of |P2pGroupCapabilityMask| indicating the group capabilities.
     * Refer to section 4.1.4 of the Wifi P2P Technical specification v1.2.
     */
    int groupCapabilities;

    /**
     * WFD device info as described in section 5.1.2 of the WFD technical
     * specification v1.0.0.
     */
    byte[] wfdDeviceInfo;

    /**
     * WFD R2 device info as described in section 5.1.12 of WFD technical
     * specification v2.1.
     */
    byte[] wfdR2DeviceInfo;

    /**
     * Vendor-specific information element bytes. The format of an
     * information element is EID (1 byte) + Length (1 Byte) + Payload which is
     * defined in Section 9.4.4 TLV encodings of 802.11-2016 IEEE Standard for
     * Information technology. The length indicates the size of the payload.
     * Multiple information elements may be appended within the byte array.
     */
    byte[] vendorElemBytes;

    /**
     * Optional vendor-specific data.
     * Null value indicates that no vendor data is provided.
     */
    @nullable OuiKeyedData[] vendorData;
}
