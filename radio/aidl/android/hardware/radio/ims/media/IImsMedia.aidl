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

package android.hardware.radio.ims.media;

import android.hardware.radio.ims.media.IImsMediaListener;
import android.hardware.radio.ims.media.LocalEndPoint;
import android.hardware.radio.ims.media.RtpConfig;
import android.hardware.radio.ims.media.RtpError;

/**
 * This interface is used by IMS media framework to talk to RTP stack located in another processor.
 * @hide
 */
@VintfStability
oneway interface IImsMedia {
    /**
     * Set the listener functions for receiving notifications from the RTP stack.
     *
     * @param mediaListener Object containing listener methods
     *
     * This is available when android.hardware.telephony.ims is defined.
     */
    void setListener(in IImsMediaListener mediaListener);

    /**
     * Opens a RTP session for the local end point with the associated initial remote configuration
     * if there is a valid RtpConfig passed. It starts the media flow if the media direction in the
     * RtpConfig is set to any value other than NO_MEDIA_FLOW. If the open session is successful
     * then the implementation shall return a new IImsMediaSession binder connection for this
     * session using IImsMediaListener#onOpenSessionSuccess() API. If the open session is failed
     * then the implementation shall return the error using IImsMediaListener#onOpenSessionFailure()
     *
     * @param sessionId unique identifier of the session
     * @param localEndPoint provides IP address, port and logical modem id for local RTP endpoint
     * @param config provides remote end point info and codec details. This could be null initially
     *        and the application may update this later using modifySession() API.
     *
     * This is available when android.hardware.telephony.ims is defined.
     */
    void openSession(int sessionId, in LocalEndPoint localEndPoint, in RtpConfig config);

    /**
     * Close the RTP session including cleanup of all the resources associated with the session.
     * This shall also close the session specific binder connection opened as part of openSession().
     *
     * @param sessionId identifier for the rtp session that needs to be closed
     *
     * This is available when android.hardware.telephony.ims is defined.
     */
    void closeSession(int sessionId);
}
