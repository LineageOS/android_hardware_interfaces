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

import android.hardware.cas.ScramblingMode;
import android.hardware.cas.SessionIntent;

/**
 * ICas is the API to control the CAS. It is used to manage sessions, provision/refresh the cas
 * system, and process the EMM/ECM messages. It also allows bi-directional, scheme-specific
 * communications between the client and the cas system.
 * @hide
 */
@VintfStability
interface ICas {
    /**
     * Close a session.
     *
     * @param sessionId The id of the session to be closed.
     */
    void closeSession(in byte[] sessionId);

    /**
     * Open a session to descramble one or more streams without specifying intention
     * and scrambling mode.
     *
     * @return sessionId The id of the newly opened session.
     */
    byte[] openSessionDefault();

    /**
     * Open a session to descramble one or more streams by specifying intention
     * and scrambling mode.
     *
     * @param intent the intention of the session to be opened.
     * @param mode the scrambling mode the session will use.
     *
     * @return sessionId The id of the newly opened session.
     */
    byte[] openSession(in SessionIntent intent, in ScramblingMode mode);

    /**
     * Process an ECM from the ECM stream for this session’s elementary stream.
     *
     * @param sessionId the id of the session which the ecm data applies to.
     * @param ecm a byte array containing the ecm data.
     */
    void processEcm(in byte[] sessionId, in byte[] ecm);

    /**
     * Process an in-band EMM from the EMM stream.
     *
     * @param emm a byte array containing the emm data.
     */
    void processEmm(in byte[] emm);

    /**
     * Initiate a provisioning operation for a CA system.
     *
     * @param provisionString string containing information needed for the
     * provisioning operation, the format of which is scheme and implementation
     * specific.
     */
    void provision(in String provisionString);

    /**
     * Notify the CA system to refresh entitlement keys.
     *
     * @param refreshType the type of the refreshment.
     * @param refreshData private data associated with the refreshment.
     */
    void refreshEntitlements(in int refreshType, in byte[] refreshData);

    /**
     * Release the descrambler instance.
     */
    void release();

    /**
     * Send an scheme-specific event to the CasPlugin.
     *
     * @param event an integer denoting a scheme-specific event to be sent.
     * @param arg a scheme-specific integer argument for the event.
     * @param data a byte array containing scheme-specific data for the event.
     */
    void sendEvent(in int event, in int arg, in byte[] eventData);

    /**
     * Send an scheme-specific session event to the CasPlugin.
     *
     * @param sessionId the id of an opened session.
     * @param event an integer denoting a scheme-specific event to be sent.
     * @param arg a scheme-specific integer argument for the event.
     * @param data a byte array containing scheme-specific data for the event.
     */
    void sendSessionEvent(in byte[] sessionId, in int event, in int arg, in byte[] eventData);

    /**
     * Provide the CA private data from a CA_descriptor in the conditional
     * access table to a CasPlugin.
     *
     * @param pvtData a byte array containing the private data, the format of
     * which is scheme-specific and opaque to the framework.
     */
    void setPrivateData(in byte[] pvtData);

    /**
     * Provide the CA private data from a CA_descriptor in the program map
     * table to a session.
     *
     * @param sessionId the id of the session which the private data applies to.
     * @param pvtData a byte array containing the private data, the format of
     * which is scheme-specific and opaque to the framework.
     */
    void setSessionPrivateData(in byte[] sessionId, in byte[] pvtData);
}
