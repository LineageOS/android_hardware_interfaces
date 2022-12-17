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

package android.hardware.cas;

import android.hardware.cas.StatusEvent;

/**
 * @hide
 */
@VintfStability
interface ICasListener {
    /**
     * Notify the listener of a scheme-specific event from the CA system.
     *
     * @param event an integer whose meaning is scheme-specific.
     * @param arg an integer whose meaning is scheme-specific.
     * @param data a byte array of data whose format and meaning are
     * scheme-specific.
     */
    void onEvent(in int event, in int arg, in byte[] data);

    /**
     * Notify the listener of a scheme-specific session event from CA system.
     *
     * @param sessionId the id of an opened session.
     * @param event an integer whose meaning is scheme-specific.
     * @param arg an integer whose meaning is scheme-specific.
     * @param data a byte array of data whose format and meaning are
     * scheme-specific.
     */
    void onSessionEvent(in byte[] sessionId, in int event, in int arg, in byte[] data);

    /**
     * Notify the listener that the status of CAS system has changed.
     *
     * @param event the event type of status change.
     * @param number value for status event.
     *               For PLUGIN_PHYSICAL_MODULE_CHANGED event:
     *               the positive number presents how many plugins are inserted;
     *               the negative number presents how many plugins are removed.
     *               Client must enumerate plugins after receive the event.
     *               For PLUGIN_SESSION_NUMBER_CHANGED event:
     *               the number presents how many sessions are supported
     *               in the plugin.
     */
    void onStatusUpdate(in StatusEvent event, in int number);
}
