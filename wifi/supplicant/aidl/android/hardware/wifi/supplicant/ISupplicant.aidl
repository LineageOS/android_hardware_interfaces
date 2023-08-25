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

import android.hardware.wifi.supplicant.DebugLevel;
import android.hardware.wifi.supplicant.INonStandardCertCallback;
import android.hardware.wifi.supplicant.ISupplicantCallback;
import android.hardware.wifi.supplicant.ISupplicantP2pIface;
import android.hardware.wifi.supplicant.ISupplicantStaIface;
import android.hardware.wifi.supplicant.IfaceInfo;
import android.hardware.wifi.supplicant.IfaceType;

/**
 * Interface exposed by the supplicant AIDL service registered
 * with the service manager. This is the root level object for
 * any of the supplicant interactions.
 */
@VintfStability
interface ISupplicant {
    /**
     * Default timeout (in seconds) for external radio work.
     */
    const int EXT_RADIO_WORK_TIMEOUT_IN_SECS = 10;

    /**
     * Registers a wireless interface in supplicant.
     *
     * @param ifName Name of the interface (e.g wlan0).
     * @return AIDL interface object representing the interface if
     *         successful, null otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_EXISTS|
     */
    @PropagateAllowBlocking ISupplicantP2pIface addP2pInterface(in String ifName);
    @PropagateAllowBlocking ISupplicantStaIface addStaInterface(in String ifName);

    /**
     * Get the debug level set.
     *
     * @return one of |DebugLevel| values.
     */
    DebugLevel getDebugLevel();

    /**
     * Gets an AIDL interface object for the interface corresponding
     * to an iface name which the supplicant already controls.
     *
     * @param ifName Name of the interface retrieved
     *        using |listInterfaces|.
     * @return AIDL interface object representing the interface if
     *         successful, null otherwise.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_UNKNOWN|
     */
    @PropagateAllowBlocking ISupplicantP2pIface getP2pInterface(in String ifName);
    @PropagateAllowBlocking ISupplicantStaIface getStaInterface(in String ifName);

    /**
     * Get whether the keys are shown in the debug logs or not.
     *
     * @return true if set, false otherwise.
     */
    boolean isDebugShowKeysEnabled();

    /**
     * Get whether the timestamps are shown in the debug logs or not.
     *
     * @return true if set, false otherwise.
     */
    boolean isDebugShowTimestampEnabled();

    /**
     * Retrieve a list of all interfaces controlled by the supplicant.
     *
     * The corresponding |ISupplicantIface| object for any interface can be
     * retrieved using the proper |getInterface| method.
     *
     * @return List of all interfaces controlled by the supplicant.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    IfaceInfo[] listInterfaces();

    /**
     * Register for callbacks from the supplicant service.
     *
     * These callbacks are invoked for global events that are not specific
     * to any interface or network. Registration of multiple callback
     * objects is supported. These objects must be deleted when the corresponding
     * client process is dead.
     *
     * @param callback An instance of the |ISupplicantCallback| AIDL interface
     *        object.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    void registerCallback(in ISupplicantCallback callback);

    /**
     * Deregisters a wireless interface from supplicant.
     *
     * @param ifaceInfo Combination of the interface type and name (e.g wlan0).
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_ARGS_INVALID|,
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|,
     *         |SupplicantStatusCode.FAILURE_IFACE_UNKNOWN|
     */
    void removeInterface(in IfaceInfo ifaceInfo);

    /**
     * Set concurrency priority.
     *
     * When both P2P and STA mode ifaces are active, this must be used
     * to prioritize either STA or P2P connection to resolve conflicts
     * arising during single channel concurrency.
     *
     * @param type The type of iface to prioritize.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    void setConcurrencyPriority(in IfaceType type);

    /**
     * Set debug parameters for the supplicant.
     *
     * @param level Debug logging level for the supplicant.
     *        (one of |DebugLevel| values).
     * @param timestamp Determines whether to show timestamps in logs or
     *        not.
     * @param showKeys Determines whether to show keys in debug logs or
     *        not.
     *        CAUTION: Do not set this param in production code!
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    void setDebugParams(in DebugLevel level, in boolean showTimestamp, in boolean showKeys);

    /**
     * Terminate the service.
     * This must de-register the service and clear all state. If this HAL
     * supports the lazy HAL protocol, then this may trigger daemon to exit and
     * wait to be restarted.
     */
    oneway void terminate();

    /**
     * Register a Non-Standard Certificate callback with supplicant.
     *
     * @param callback An instance of the |INonStandardCertCallback| AIDL interface
     *        object.
     * @throws ServiceSpecificException with one of the following values:
     *         |SupplicantStatusCode.FAILURE_UNKNOWN|
     */
    void registerNonStandardCertCallback(in INonStandardCertCallback callback);
}
