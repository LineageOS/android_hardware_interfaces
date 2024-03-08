/*
 * Copyright (C) 2022 The Android Open Source Project
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
parcelable CallQuality {
    /**
     * downlink CallQualityLevel for a given ongoing call
     * this value corresponds to the CALL_QUALITY_* constants in {@link CallQuality}
     */
    int downlinkCallQualityLevel;
    /**
     * uplink CallQualityLevel for a given ongoing call
     * this value corresponds to the CALL_QUALITY_* constants in {@link CallQuality}
     */
    int uplinkCallQualityLevel;
    /** the call duration in milliseconds */
    int callDuration;
    /** RTP packets sent to network */
    int numRtpPacketsTransmitted;
    /** RTP packets received from network */
    int numRtpPacketsReceived;
    /** RTP packets which were lost in network and never transmitted */
    int numRtpPacketsTransmittedLost;
    /** RTP packets which were lost in network and never received */
    int numRtpPacketsNotReceived;
    /** average relative jitter in milliseconds */
    int averageRelativeJitter;
    /** maximum relative jitter in milliseconds */
    int maxRelativeJitter;
    /** average round trip delay in milliseconds */
    int averageRoundTripTime;
    /**
     * the codec type. This value corresponds to the AUDIO_QUALITY_* constants in
     * {@link ImsStreamMediaProfile}
     */
    int codecType;
    /** True if no incoming RTP is received for a continuous duration of 4 seconds */
    boolean rtpInactivityDetected;
    /**
     * True if only silence RTP packets are received for 20 seconds
     * immediately after call is connected
     */
    boolean rxSilenceDetected;
    /**
     * True if only silence RTP packets are sent for 20 seconds
     * immediately after call is connected
     */
    boolean txSilenceDetected;
    /** the number of Voice frames sent by jitter buffer to audio */
    int numVoiceFrames;
    /** the number of no-data frames sent by jitter buffer to audio */
    int numNoDataFrames;
    /** the number of RTP voice packets dropped by jitter buffer */
    int numDroppedRtpPackets;
    /** the minimum playout delay in the reporting interval in milliseconds */
    long minPlayoutDelayMillis;
    /** the maximum playout delay in the reporting interval in milliseconds */
    long maxPlayoutDelayMillis;
    /**
     * the total number of RTP SID (Silence Insertion Descriptor) packets
     * received by this device for an ongoing call
     */
    int numRtpSidPacketsReceived;
    /**
     * the total number of RTP duplicate packets received by this device
     * for an ongoing call
     */
    int numRtpDuplicatePackets;
}
