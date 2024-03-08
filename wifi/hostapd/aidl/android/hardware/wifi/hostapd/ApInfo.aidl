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

package android.hardware.wifi.hostapd;

import android.hardware.wifi.common.OuiKeyedData;
import android.hardware.wifi.hostapd.ChannelBandwidth;
import android.hardware.wifi.hostapd.Generation;

/**
 * Parameters to control the channel selection for the interface.
 */
@VintfStability
parcelable ApInfo {
    /**
     * Name of the interface which was added via |IHostapd.addAccessPoint|.
     */
    String ifaceName;

    /**
     * The identity of the AP instance. The interface will have two instances
     * (e.g. 2.4 Ghz AP and 5 GHz AP) in dual AP mode.
     * The apIfaceInstance can be used to identify which instance the callback
     * is from.
     * Note: The apIfaceInstance must be same as ifaceName in single AP mode.
     */
    String apIfaceInstance;

    /**
     * The operational frequency of the AP in Mhz.
     */
    int freqMhz;

    /**
     * The operational channel bandwidth of the AP.
     */
    ChannelBandwidth channelBandwidth;

    /**
     * The operational mode of the AP (e.g. 11ac, 11ax).
     */
    Generation generation;

    /**
     * MAC Address of the apIfaceInstance.
     */
    byte[] apIfaceInstanceMacAddress;

    /**
     * Optional vendor-specific information.
     */
    @nullable OuiKeyedData[] vendorData;
}
