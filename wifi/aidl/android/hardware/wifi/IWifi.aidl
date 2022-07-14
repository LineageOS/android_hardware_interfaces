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

import android.hardware.wifi.IWifiChip;
import android.hardware.wifi.IWifiEventCallback;

/**
 * This is the root of the HAL module and is the interface returned when
 * loading an implementation of the Wi-Fi HAL. There must be at most one
 * module loaded in the system.
 */
@VintfStability
interface IWifi {
    /**
     * Gets an AIDL interface object for the chip corresponding to the
     * provided chipId.
     *
     * @param chipId ID of the chip.
     * @return AIDL interface object representing the chip.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.NOT_STARTED|,
     *         |WifiStatusCode.UNKNOWN|
     */
    @PropagateAllowBlocking IWifiChip getChip(int chipId);

    /**
     * Retrieves the list of all chip id's on the device.
     * The corresponding |IWifiChip| object for any chip can be
     * retrieved using the |getChip| method.
     *
     * @return List of all chip id's on the device.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.NOT_STARTED|,
     *         |WifiStatusCode.UNKNOWN|
     */
    int[] getChipIds();

    /**
     * Get the current state of the HAL.
     *
     * @return true if started, false otherwise.
     */
    boolean isStarted();

    /**
     * Requests notifications of significant HAL events. Multiple calls to
     * this must register multiple callbacks, each of which must receive all
     * events. |IWifiEventCallback| object registration must be independent of the
     * state of the rest of the HAL and must persist though stops/starts. These
     * objects must be deleted when the corresponding client process is dead.
     *
     * @param callback An instance of the |IWifiEventCallback| AIDL interface
     *        object.
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.UNKNOWN|
     */
    void registerEventCallback(in IWifiEventCallback callback);

    /**
     * Perform any setup that is required to make use of the module.
     * If the module is already started then this must be a noop.
     * Must trigger |IWifiEventCallback.onStart| on success.
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.NOT_AVAILABLE|,
     *         |WifiStatusCode.UNKNOWN|
     */
    void start();

    /**
     * Tear down any state, ongoing commands, etc. If the module is already
     * stopped then this must be a noop. After calling this, all |IWifiChip|
     * objects will be considered invalid.
     * Must trigger |IWifiEventCallback.onStop| on success.
     * Must trigger |IWifiEventCallback.onFailure| on failure.
     *
     * Calling stop() and then start() is a valid way of resetting state in
     * the HAL, driver, and firmware.
     *
     * @throws ServiceSpecificException with one of the following values:
     *         |WifiStatusCode.NOT_STARTED|,
     *         |WifiStatusCode.UNKNOWN|
     */
    void stop();
}
