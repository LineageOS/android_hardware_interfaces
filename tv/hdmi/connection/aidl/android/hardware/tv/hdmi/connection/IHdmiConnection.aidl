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

package android.hardware.tv.hdmi.connection;

import android.hardware.tv.hdmi.connection.HdmiPortInfo;
import android.hardware.tv.hdmi.connection.HpdSignal;
import android.hardware.tv.hdmi.connection.IHdmiConnectionCallback;

/**
 * HDMI Connection HAL interface definition.
 */
@VintfStability
interface IHdmiConnection {
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
    void setCallback(in IHdmiConnectionCallback callback);

    /**
     * Method to set the HPD (Hot Plug Detection) signal the HAL should use for HPD signaling (e.g.
     * signaling EDID updates). By default, the HAL will use {@code HDMI_HPD_PHYSICAL} (the physical
     * hotplug signal). When set to {@code HDMI_HPD_STATUS_BIT} the HAL should use the HDP status
     * bit.
     *
     * This is only relevant to TV Panel devices that support eARC TX. While eARC TX is connected,
     * the framework calls this method to set the HPD signal to {@code HDMI_HPD_STATUS_BIT}.
     *
     * For all other device types, this method can be stubbed.
     *
     * @param signal The HPD signal type to use.
     * @param portId id of the port on which the HPD signal should be set.
     *
     * @throws ServiceSpecificException with error code set to
     *         {@code Result::FAILURE_NOT_SUPPORTED} if the signal type is not supported.
     *         {@code Result::FAILURE_INVALID_ARGS} if the signal type is invalid.
     *         {@code Result::FAILURE_UNKNOWN} if the signal type could not be set because of an
     *                                         unknown failure.
     */
    void setHpdSignal(HpdSignal signal, in int portId);

    /**
     * Get the current signal the HAL is using for HPD
     *
     * This is only relevant to TV Panel devices that support eARC TX. While eARC TX is connected,
     * this method returns {@code HDMI_HPD_STATUS_BIT}.
     *
     * For all other device types, this method can be stubbed by always returning
     * {@code HDMI_HPD_PHYSICAL}.
     *
     * @param portId id of the port of which the current HPD signal is queried.
     */
    HpdSignal getHpdSignal(in int portId);
}
