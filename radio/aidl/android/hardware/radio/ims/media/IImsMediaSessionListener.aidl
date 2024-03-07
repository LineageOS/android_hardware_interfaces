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

import android.hardware.radio.ims.media.CallQuality;
import android.hardware.radio.ims.media.MediaQualityStatus;
import android.hardware.radio.ims.media.RtpConfig;
import android.hardware.radio.ims.media.RtpError;
import android.hardware.radio.ims.media.RtpHeaderExtension;
import android.hardware.radio.ims.media.RtpReceptionStats;

/**
 * Interface declaring listener functions for unsolicited IMS media notifications per session.
 * @hide
 */
@VintfStability
oneway interface IImsMediaSessionListener {
    /**
     * Notifies the result of IImsMediaSession#modifySession() API.
     *
     * @param config The RTP config passed in IImsMediaSession#modifySession() API
     * @param error RtpError.NONE in case of success else one of the following
     *   RtpError :INVALID_PARAM
     *   RtpError :INTERNAL_ERR
     *   RtpError :NO_MEMORY
     *   RtpError :NO_RESOURCES
     *
     * This is available when android.hardware.telephony.ims is defined.
     */
    void onModifySessionResponse(in RtpConfig config, RtpError error);

    /**
     * Indicates when the first Rtp media packet is received by the UE during ring
     * back, call hold or early media scenarios. This is sent only if the packet is
     * received on the active remote configuration.
     *
     * In case of early media scenarios, the implementation shall play the RTP
     * packets from the most recently added config.
     *
     * @param config The remote config where the media is received
     *
     * This is available when android.hardware.telephony.ims is defined.
     */
    void onFirstMediaPacketReceived(in RtpConfig config);

    /**
     * RTP header extension received from the other party
     *
     * @param extensions content of the received RTP header extension
     *
     * This is available when android.hardware.telephony.ims is defined.
     */
    void onHeaderExtensionReceived(in List<RtpHeaderExtension> extensions);

    /**
     * Notifies when the measured media quality crosses at least one of
     * {@link MediaQualityThreshold} set by {@link IImsMediaSession#setMediaQualityThreshold()}.
     *
     * @param quality The object of MediaQualityStatus with the rtp and the rtcp statistics.
     *
     * This is available when android.hardware.telephony.ims is defined.
     */
    void notifyMediaQualityStatus(in MediaQualityStatus quality);

    /**
     * The modem RTP stack fires this API to query whether the desired bitrate mentioned
     * in the RtpConfig is currently available on the NW or not using ANBRQ message.
     * See 3GPP TS 26.114.
     *
     * @param config containing desired bitrate and direction
     *
     * This is available when android.hardware.telephony.ims is defined.
     */
    void triggerAnbrQuery(in RtpConfig config);

    /**
     * Notifies the received DTMF digit from the other party
     *
     * @param dtmfDigit single char having one of 12 values: 0-9, *, #
     * @param durationMs The duration to play the tone in milliseconds unit
     *
     * This is available when android.hardware.telephony.ims is defined.
     */
    void onDtmfReceived(char dtmfDigit, int durationMs);

    /**
     * Notifies when a change to call quality has occurred
     *
     * @param CallQuality The call quality statistics of ongoing call since last report
     *
     * This is available when android.hardware.telephony.ims is defined.
     */
    void onCallQualityChanged(in CallQuality callQuality);

    /**
     * Notifies the RTP reception statistics periodically after
     * IImsMediaSession#requestRtpReceptionStats(intervalMs) is invoked.
     *
     * @param stats The RTP reception statistics
     *
     * This is available when android.hardware.telephony.ims is defined.
     */
    void notifyRtpReceptionStats(in RtpReceptionStats stats);
}
