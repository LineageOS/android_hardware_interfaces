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

package android.hardware.tv.hdmi;

import android.hardware.tv.hdmi.HdmiPortInfo;
import android.hardware.tv.hdmi.IHdmiCallback;

/**
 * HDMI HAL interface definition.
 */
@VintfStability
interface IHdmi {
    /**
     * Gets the hdmi port information of underlying hardware.
     *
     * @return The list of HDMI port information
     */
    HdmiPortInfo[] getPortInfo();

    /**
     * Gets the connection status of the specified port.
     *
     * @param portId Port id to be inspected for the connection status.
     * @return True if a device is connected, otherwise false. The HAL
     *         must watch for +5V power signal to determine the status.
     */
    boolean isConnected(in int portId);

    /**
     * Sets a callback that HDMI HAL must later use for internal HDMI events
     *
     * @param callback Callback object to pass hdmi events to the system. The
     *        previously registered callback must be replaced with this one.
     *        setCallback(null) should deregister the callback.
     */
    void setCallback(in IHdmiCallback callback);
}
