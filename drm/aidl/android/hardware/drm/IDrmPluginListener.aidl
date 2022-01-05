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

package android.hardware.drm;

import android.hardware.drm.EventType;
import android.hardware.drm.KeyStatus;

/**
 * IDrmPluginListener is a listener interface for Drm events sent from an
 * IDrmPlugin instance.
 */
@VintfStability
interface IDrmPluginListener {
    /**
     * Legacy event sending method, it sends events of various types using a
     * single overloaded set of parameters. This form is deprecated.
     *
     * @param eventType the type of the event
     * @param sessionId identifies the session the event originated from
     * @param data event-specific data blob
     */
    oneway void onEvent(in EventType eventType, in byte[] sessionId, in byte[] data);

    /**
     * Send a license expiration update to the listener. The expiration
     * update indicates how long the current keys are valid before they
     * need to be renewed.
     *
     * @param sessionId identifies the session the event originated from
     * @param expiryTimeInMS the time when the keys need to be renewed.
     * The time is in milliseconds, relative to the Unix epoch. A time
     * of 0 indicates that the keys never expire.
     */
    oneway void onExpirationUpdate(in byte[] sessionId, in long expiryTimeInMS);

    /**
     * Send a keys change event to the listener. The keys change event
     * indicates the status of each key in the session. Keys can be
     * indicated as being usable, expired, outputnotallowed or statuspending.
     *
     * @param sessionId identifies the session the event originated from
     * @param keyStatusList indicates the status for each key ID in the
     * session.
     * @param hasNewUsableKey indicates if the event includes at least one
     * key that has become usable.
     */
    oneway void onKeysChange(
            in byte[] sessionId, in KeyStatus[] keyStatusList, in boolean hasNewUsableKey);

    /**
     * Some device crypto hardware is incapable of retaining crypto
     * session state across suspend and resume cycles. A
     * SessionLostState event must be signaled when a session has
     * become invalid for this reason. This event must not be used to
     * indicate a failure in the crypto system. Closing the session
     * and opening a new one must allow the application to resume
     * normal use of the drm hal module.
     *
     * @param sessionId identifies the session that has been invalidated
     */
    oneway void onSessionLostState(in byte[] sessionId);
}
