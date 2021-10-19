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

import android.hardware.radio.ims.media.IImsMediaSession;
import android.hardware.radio.ims.media.MediaStackState;
import android.hardware.radio.ims.media.RtpError;
import android.hardware.radio.ims.media.SpeechCodec;

/**
 * Interface declaring listener functions for unsolicited IMS media notifications.
 */
@VintfStability
oneway interface IImsMediaListener {
    /**
     * Notifies the media stack bring up status and capabilities.
     *
     * @param status bring up status
     * @param supportedCodecs array of supported speech codecs by the implementation
     */
    void onBringupResponse(RtpError status, in SpeechCodec[] supportedCodecs);

    /**
     * Notifies the state of the media stack whenever there is a change.
     *
     * @param state current state of the media stack
     */
    void onMediaStackStateChanged(MediaStackState state);

    /**
     * Fired when a IImsMedia#openSession() API is successful.
     *
     * @param sessionId identifier of the session
     * @param session new IImsMediaSession binder connection to be used for the session
     *        specific operations
     */
    void onOpenSessionSuccess(int sessionId, IImsMediaSession session);

    /**
     * Fired when IImsMedia#openSession() API failed to create a new session.
     *
     * @param sessionId identifier of the session
     * @param error one of the following RTP error code
     *   RtpError :INVALID_PARAM
     *   RtpError :INTERNAL_ERR
     *   RtpError :NO_MEMORY
     *   RtpError :NO_RESOURCES
     *   RtpError :PORT_UNAVAILABLE
     */
     void onOpenSessionFailure(int sessionId, RtpError error);
}
