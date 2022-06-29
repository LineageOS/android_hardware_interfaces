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

import android.hardware.wifi.hostapd.ApInfo;
import android.hardware.wifi.hostapd.ClientInfo;

/**
 * Top-level callback interface for managing SoftAPs.
 */
@VintfStability
interface IHostapdCallback {
    /**
     * Invoked when information changes for one of the AP instances.
     *
     * @param apInfo AP information of the instance changed.
     */
    oneway void onApInstanceInfoChanged(in ApInfo apInfo);

    /**
     * Invoked when a client connects/disconnects from the hotspot.
     *
     */
    oneway void onConnectedClientsChanged(in ClientInfo clientInfo);

    /**
     * Invoked when an asynchronous failure is encountered in one of the access
     * points added via |IHostapd.addAccessPoint|.
     *
     * @param ifaceName Name of the interface which was added via
     *                  |IHostapd.addAccessPoint|.
     * @param instanceName Name of the AP instance which is associated with
     *                     the interface.
     */
    oneway void onFailure(in String ifaceName, in String instanceName);
}
