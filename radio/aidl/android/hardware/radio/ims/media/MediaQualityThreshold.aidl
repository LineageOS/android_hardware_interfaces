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

@VintfStability
parcelable MediaQualityThreshold {
    /** Timer in milliseconds for monitoring RTP inactivity */
    int rtpInactivityTimerMillis;
    /** Timer in milliseconds for monitoring RTCP inactivity */
    int rtcpInactivityTimerMillis;
    /** Duration in milliseconds for monitoring the RTP packet loss rate */
    int rtpPacketLossDurationMillis;
    /**
     * Packet loss rate in percentage of (total number of packets lost) /
     * (total number of packets expected) during rtpPacketLossDurationMs
     */
    int rtpPacketLossRate;
    /** Duration in milliseconds for monitoring the jitter for RTP traffic */
    int jitterDurationMillis;
    /** RTP jitter threshold in milliseconds */
    int rtpJitterMillis;
}
