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

import android.hardware.radio.AccessNetwork;
import android.hardware.radio.ims.media.AnbrMode;
import android.hardware.radio.ims.media.RtcpConfig;
import android.hardware.radio.ims.media.RtpAddress;
import android.hardware.radio.ims.media.RtpSessionParams;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable RtpConfig {
    /** Media flow direction. The bitfield of MediaDirection(s) */
    int direction;
    /** Radio Access Network */
    AccessNetwork accessNetwork;
    /** IP address and port number of the other party for RTP media */
    RtpAddress remoteAddress;
    /** Negotiated session parameters */
    RtpSessionParams sessionParams;
    /** RTCP configuration */
    RtcpConfig rtcpConfig;
    /**
     * ANBR Mode parameters. This is set to valid only when its triggered,
     * otherwise it shall be set to NULL.
     *
     * This would be used in the following two cases
     * - IImsMediaSession#modifySession(RtpConfig) - When RAN wants to change the bit
     *   rate via ANBR MAC layer signaling, ImsStack converts the received bitrate
     *   to the codec mode appropriately and passes the codec mode and direction
     *   using modifySession(). The underlying RTP stack shall adapt to
     *   the changed bitrate.
     *
     * - IImsMediaSessionListener#triggerAnbrQuery(RtpConfig) - When the vendor RTP
     *   stack receives a request for bitrate increase from the peer terminal via CMR,
     *   RTCP-APP or TMMBR, it triggers ANBRQ by setting the desired bitrate and the
     *   direction of the stream.
     */
    AnbrMode anbrModeParams;
}
