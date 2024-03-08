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

package android.hardware.wifi.supplicant;

import android.hardware.wifi.common.OuiKeyedData;
import android.hardware.wifi.supplicant.LegacyMode;
import android.hardware.wifi.supplicant.WifiTechnology;

/**
 * Connection Capabilities supported by current network and device
 */
@VintfStability
parcelable ConnectionCapabilities {
    /**
     * Wifi Technology
     */
    WifiTechnology technology;
    /**
     * channel bandwidth
     */
    int channelBandwidth;
    /**
     * max number of Tx spatial streams
     */
    int maxNumberTxSpatialStreams;
    /**
     * max number of Rx spatial streams
     */
    int maxNumberRxSpatialStreams;
    /**
     * detailed network mode for legacy network
     */
    LegacyMode legacyMode;
    /**
     * Indicates the AP support for TID-to-link mapping negotiation.
     */
    boolean apTidToLinkMapNegotiationSupported;
    /**
     * Additional vendor-specific data. Null value indicates
     * that no vendor data is provided.
     */
    @nullable OuiKeyedData[] vendorData;
}
