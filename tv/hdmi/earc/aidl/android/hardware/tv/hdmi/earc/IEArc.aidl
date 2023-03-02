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

package android.hardware.tv.hdmi.earc;

import android.hardware.tv.hdmi.earc.IEArcCallback;
import android.hardware.tv.hdmi.earc.IEArcStatus;

/**
 * eARC HAL interface definition. This is only relevant to TV Panels that implement eARC TX.
 */
@VintfStability
interface IEArc {
    /**
     * Function to enable or disable eARC in the device's driver and HAL. If enabled, the driver and
     * HAL shall attempt to establish an eARC connection and inform the Android framework about
     * updates with IEArcCallback callbacks. If disabled, the driver and HAL shall not attempt to
     * establish an eARC connection and shall not send any IEArcCallback callbacks to the Android
     * framework.
     * @throws ServiceSpecificException with error code set to
     *         {@code Result::FAILURE_NOT_SUPPORTED} if the eARC enabled setting could not be set
     *                                               because this is not supported.
     *         {@code Result::FAILURE_INVALID_ARGS} if the eARC enabled setting could not be set
     *                                              because the method argument is invalid.
     *         {@code Result::FAILURE_UNKNOWN} if the eARC enabled setting could not be set because
     *                                         there was an unknown failure.
     */
    void setEArcEnabled(in boolean enabled);

    /**
     * Function to check if eARC is enabled in the device's driver and HAL.
     */
    boolean isEArcEnabled();

    /**
     * Function to set callback that the HAL will use to notify the system of connection state
     * changes and capabilities of connected devices.
     *
     * @param callback The callback object to pass the events to the system. A previously registered
     *        callback should be replaced by this new object. If callback is {@code null} the
     *        previously registered callback should be deregistered.
     */
    void setCallback(in IEArcCallback callback);

    /**
     * Getter for the current eARC state of a port.
     *
     * @param portId The port ID for which the state is to be reported.
     * @return The state of the port.
     */
    IEArcStatus getState(in int portId);

    /**
     * Getter for the most recent audio capabilities reported by the device connected to port.
     *
     * @param portId The port ID on which the device is connected.
     * @return a byte array containing the raw, unparsed audio capabilities (Ref "Section 9.5.3.6 -
     * eARC RX Capabilities Data Structure" in HDMI 2.1 specification).
     */
    byte[] getLastReportedAudioCapabilities(in int portId);
}
