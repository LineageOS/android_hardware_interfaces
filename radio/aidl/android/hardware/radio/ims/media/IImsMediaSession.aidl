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

import android.hardware.radio.ims.media.IImsMediaSessionListener;
import android.hardware.radio.ims.media.MediaQualityThreshold;
import android.hardware.radio.ims.media.RtpConfig;
import android.hardware.radio.ims.media.RtpError;
import android.hardware.radio.ims.media.RtpHeaderExtension;

/**
 * Session specific interface used by IMS media framework to talk to the vendor RTP stack.
 */
@VintfStability
oneway interface IImsMediaSession {
    /**
     * Set the listener functions to receive IMS media session specific notifications.
     *
     * @param sessionListener Object containing notification methods
     */
    void setListener(in IImsMediaSessionListener sessionListener);

    /**
     * Modifies the configuration of the RTP session. It can be used to pause/resume
     * the media stream by changing the value of the MediaDirection.
     *
     * @param config provides remote end point info and codec details
     */
    void modifySession(in RtpConfig config);

    /**
     * Send DTMF digit until the duration expires.
     *
     * @param dtmfDigit single char having one of 12 values: 0-9, *, #
     * @param duration of the key press in milliseconds.
     */
    void sendDtmf(char dtmfDigit, int duration);

    /**
     * Start sending DTMF digit until the stopDtmf() API is received.
     * If the implementation is currently sending a DTMF tone for which
     * stopDtmf() is not received yet, then that digit must be stopped first
     *
     * @param dtmfDigit single char having one of 12 values: 0-9, *, #
     */
    void startDtmf(char dtmfDigit);

    /**
     * Stop sending the last DTMF digit started by startDtmf().
     * stopDtmf() without preceding startDtmf() must be ignored.
     */
    void stopDtmf();

    /**
     * Send RTP header extension to the other party in the next RTP packet.
     *
     * @param extensions data to be transmitted via RTP header extension
     */
    void sendHeaderExtension(in List<RtpHeaderExtension> extensions);

    /**
     * Sets the media quality threshold parameters of the session to get
     * media quality notifications.
     *
     * @param threshold media quality thresholds for various quality parameters
     */
    void setMediaQualityThreshold(in MediaQualityThreshold threshold);
}
