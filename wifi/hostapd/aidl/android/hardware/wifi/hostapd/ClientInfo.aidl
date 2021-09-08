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

/**
 * Parameters to control the channel selection for the interface.
 */
@VintfStability
parcelable ClientInfo {
    /**
     * Name of the interface which was added via |IHostapd.addAccessPoint|.
     */
    String ifaceName;

    /**
     * The identity of the AP instance. The interface will have two instances in dual AP mode.
     * The apIfaceInstance can be used to identify which instance the callback is from.
     * Note: The apIfaceInstance must be same as ifaceName in single AP mode.
     */
    String apIfaceInstance;

    /**
     * MAC Address of hotspot client.
     */
    byte[] clientAddress;

    /**
     * True when client connected, false when client disconnected.
     */
    boolean isConnected;
}
