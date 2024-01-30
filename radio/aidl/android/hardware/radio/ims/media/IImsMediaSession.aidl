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
 * @hide
 */
@VintfStability
oneway interface IImsMediaSession {
    /**
     * Set the listener functions to receive IMS media session specific notifications.
     *
     * @param sessionListener Object containing notification methods
     *
     * This is available when android.hardware.telephony.ims is defined.
     */
    void setListener(in IImsMediaSessionListener sessionListener);

    /**
     * Modifies the configuration of the RTP session. It can be used to pause/resume
     * the media stream by changing the value of the MediaDirection.
     *
     * @param config provides remote end point info and codec details
     *
     * This is available when android.hardware.telephony.ims is defined.
     */
    void modifySession(in RtpConfig config);

    /**
     * Send DTMF digit until the duration expires.
     *
     * @param dtmfDigit single char having one of 12 values: 0-9, *, #
     * @param duration of the key press in milliseconds.
     *
     * This is available when android.hardware.telephony.ims is defined.
     */
    void sendDtmf(char dtmfDigit, int duration);

    /**
     * Start sending DTMF digit until the stopDtmf() API is received.
     * If the implementation is currently sending a DTMF tone for which
     * stopDtmf() is not received yet, then that digit must be stopped first
     *
     * @param dtmfDigit single char having one of 12 values: 0-9, *, #
     *
     * This is available when android.hardware.telephony.ims is defined.
     */
    void startDtmf(char dtmfDigit);

    /**
     * Stop sending the last DTMF digit started by startDtmf().
     * stopDtmf() without preceding startDtmf() must be ignored.
     *
     * This is available when android.hardware.telephony.ims is defined.
     */
    void stopDtmf();

    /**
     * Send RTP header extension to the other party in the next RTP packet.
     *
     * @param extensions data to be transmitted via RTP header extension
     *
     * This is available when android.hardware.telephony.ims is defined.
     */
    void sendHeaderExtension(in List<RtpHeaderExtension> extensions);

    /**
     * Sets the media quality threshold parameters of the session to get
     * media quality notifications.
     *
     * @param threshold media quality thresholds for various quality parameters
     *
     * This is available when android.hardware.telephony.ims is defined.
     */
    void setMediaQualityThreshold(in MediaQualityThreshold threshold);

    /**
     * Queries the current RTP reception statistics of the RTP stream. It will trigger the
       IImsMediaSessionListener#notifyRtpReceptionStats(RtpReceptionStats).
     *
     * @param intervalMs The interval of the time in milliseconds of the RTP reception
     * notification. When it is zero, the report is disabled.
     *
     * This is available when android.hardware.telephony.ims is defined.
     */
    void requestRtpReceptionStats(in int intervalMs);

    /**
     * Adjust the delay in the jitter buffer to synchronize the audio with the time of video
     * frames
     *
     * @param delayMs The delay to apply to the jitter buffer. If it is positive, the jitter
     * buffer increases the delay, if it is negative, the jitter buffer decreases the delay.
     *
     * This is available when android.hardware.telephony.ims is defined.
     */
    void adjustDelay(in int delayMs);
}
