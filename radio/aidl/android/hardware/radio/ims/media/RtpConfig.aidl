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

import android.hardware.radio.data.QosBandwidth;
import android.hardware.radio.ims.media.MediaDirection;
import android.hardware.radio.ims.media.RtcpConfig;
import android.hardware.radio.ims.media.RtpAddress;
import android.hardware.radio.ims.media.RtpSessionParams;

@VintfStability
parcelable RtpConfig {
    /** Unique identifier of the RTP config within a session */
    int configId;
    /** Media flow direction */
    MediaDirection direction;
    /** IP address and port number of the other party */
    RtpAddress remoteAddress;
    /** Negotiated session parameters */
    RtpSessionParams sessionParams;
    /** RTCP configuration */
    RtcpConfig rtcpConfig;
    /** Downlink bandwidth allocated by network in the dedicated bearer */
    QosBandwidth downlink;
    /** Uplink bandwidth allocated by network in the dedicated bearer */
    QosBandwidth uplink;
}
