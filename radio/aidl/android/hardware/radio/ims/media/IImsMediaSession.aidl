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
import android.hardware.radio.ims.media.MediaProtocolType;
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
     * Adds a new remote configuration to a RTP session during early media
     * scenarios where the IMS network could add more than one remote endpoint.
     *
     * @param config provides remote end point info and codec details
     */
    void addConfig(in RtpConfig config);

    /**
     * Deletes a remote configuration from a RTP session during early media
     * scenarios. A session shall have at least one config so this API shall
     * not delete the last config.
     *
     * @param config remote config to be deleted
     */
    void deleteConfig(in RtpConfig config);

    /**
     * Confirms a remote configuration for a Rtp session for early media scenarios
     * when there are more than one remote configs. All other early remote configs
     * (potentially including the config created as part of openSession) are auto
     * deleted when one config is confirmed.
     * Confirming a remote configuration is necessary only if additional
     * configurations were created.
     * New remote configurations cannot be added after a remote configuration is
     * confirmed.
     *
     * @param config remote config to be confirmed
     */
    void confirmConfig(in RtpConfig config);

    /**
     * Start sending DTMF digit until the duration expires or a stopDtmf() API
     * is received. If the implementation is currently playing a DTMF tone, that
     * tone must be stopped first using stopDtmf().
     *
     * @param dtmfDigit single char having one of 12 values: 0-9, *, #
     * @param volume of the DTMF digit between 0 and -63 dBm dropping the sign.
     * @param duration of the key press in milliseconds. -1 means no duration
     *        is passed and the caller will invoke stopDtmf().
     */
    void startDtmf(char dtmfDigit, int volume, int duration);

    /** Stop sending the last DTMF digit started by startDtmf. */
    void stopDtmf();

    /**
     * Send RTP header extension to the other party in the next RTP packet.
     *
     * @param data data to be transmitted via RTP header extension
     */
    void sendHeaderExtension(in RtpHeaderExtension[] data);

    /**
     * Sets the media quality threshold parameters of the session to get
     * media quality notifications.
     *
     * @param threshold media quality thresholds for various quality parameters
     */
    void setMediaQualityThreshold(in MediaQualityThreshold threshold);
}
