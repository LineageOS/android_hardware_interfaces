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

import android.hardware.radio.ims.media.MediaProtocolType;
import android.hardware.radio.ims.media.RtpHeaderExtension;
import android.hardware.radio.ims.media.RtpConfig;
import android.hardware.radio.ims.media.RtpError;
import android.hardware.radio.ims.media.RtpSession;

/**
 * Interface declaring listener functions for unsolicited IMS media notifications per session.
 */
@VintfStability
oneway interface IImsMediaSessionListener {
    /**
     * Notifies whenever a change occurs to the RTP session.
     *
     * @param session Updated RTP session
     */
     void onSessionChanged(in RtpSession session);

    /**
     * Notifies the result of IImsMediaSession#modifySession() API.
     *
     * @param config The RTP config passed in IImsMediaSession#modifySession() API
     * @param error RtpError.NONE in case of success else one of the following
     *   RtpError :INVALID_PARAM
     *   RtpError :INTERNAL_ERR
     *   RtpError :NO_MEMORY
     *   RtpError :NO_RESOURCES
     */
    void onModifySessionResponse(in RtpConfig config, RtpError error);

    /**
     * Notifies the result of IImsMediaSession#addConfig() API.
     *
     * @param config The RTP config passed in IImsMediaSession#addConfig() API
     * @param error RtpError.NONE in case of success else one of the following
     *   RtpError :INVALID_PARAM
     *   RtpError :INTERNAL_ERR
     *   RtpError :NO_MEMORY
     *   RtpError :NO_RESOURCES
     *   RtpError :PORT_UNAVAILABLE
     */
    void onAddConfigResponse(in RtpConfig config, RtpError error);

    /**
     * Notifies the result of IImsMediaSession#confirmConfig() API.
     *
     * @param config The RtpConfig passed in IImsMediaSession#confirmConfig() API
     * @param error RtpError.NONE in case of success else one of the following
     *   RtpError :INVALID_PARAM
     *   RtpError :INTERNAL_ERR
     *   RtpError :NO_RESOURCES
     */
    void onConfirmConfigResponse(in RtpConfig config, RtpError error);

    /**
     * Indicates when the first Rtp media packet is received by the UE during ring
     * back, call hold or early media scenarios. This is sent only if the packet is
     * received on the active remote configuration.
     *
     * In case of early media scenarios, the implementation shall play the RTP
     * packets from the most recently added config.
     *
     * @param config The remote config where the media is received
     */
    void onFirstMediaPacketReceived(in RtpConfig config);

    /**
     * RTP header extension received from the other party
     *
     * @param data content of the received RTP header extension
     */
     void onHeaderExtensionReceived(in RtpHeaderExtension[] data);

    /**
     * Notifies media inactivity observed as per thresholds set by
     * setMediaQualityThreshold() API
     *
     * @param packetType either RTP or RTCP
     * @param duration Inactivity duration
     */
     void notifyMediaInactivity(MediaProtocolType packetType, int duration);

    /**
     * Notifies RTP packet loss observed as per thresholds set by
     * setMediaQualityThreshold() API
     *
     * @param packetLossPercentage percentage of packet loss calculated over the duration
     */
     void notifyPacketLoss(int packetLossPercentage);

    /**
     * Notifies RTP jitter observed as per thresholds set by
     * IImsMediaSession#setMediaQualityThreshold() API
     *
     * @param jitter jitter of the RTP packets in milliseconds calculated over the duration
     */
     void notifyJitter(int jitter);
}
