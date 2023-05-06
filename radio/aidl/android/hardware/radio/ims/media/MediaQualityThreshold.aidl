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

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable MediaQualityThreshold {
    /** Array including threshold values in milliseconds for monitoring RTP inactivity */
    int[] rtpInactivityTimerMillis;
    /** Timer in milliseconds for monitoring RTCP inactivity */
    int rtcpInactivityTimerMillis;
    /**
     * This value determines the size of the time window (in milliseconds)
     * required to calculate the packet loss rate per second for the manner of
     * smoothly monitoring changes in the packet loss rate value.
     */
    int rtpPacketLossDurationMillis;
    /**
     * The threshold hysteresis time for packet loss and jitter. This has a goal to prevent
     * frequent ping-pong notification. So whenever a notifier needs to report the cross of
     * threshold in opposite direction, this hysteresis timer should be respected.
     */
    int rtpHysteresisTimeInMillis;
    /**
     * Array including threshold values of Packet loss rate in percentage of
     * (total number of packets lost) / (total number of packets expected) calculated
     * every one sec with the packet received in rtpPacketLossDurationMillis.
     */
    int[] rtpPacketLossRate;

    /** Array including threshold values in milliseconds for RTP jitter */
    int[] rtpJitterMillis;

    /**
     * A flag indicating whether the client needs to be notified the current media quality status
     * right after the threshold is being set. True means the media stack should notify the client
     * of the current status.
     */
    boolean notifyCurrentStatus;
}
