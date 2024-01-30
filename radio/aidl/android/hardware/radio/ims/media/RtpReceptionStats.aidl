/*
 * Copyright (C) 2023 The Android Open Source Project
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
parcelable RtpReceptionStats {
    /** The timestamp of the latest RTP packet received */
    int rtpTimestamp;
    /** The sequence number of latest RTP packet received */
    int rtpSequenceNumber;
    /** The system clock time in millisecond of latest RTP packet received */
    int timeDurationMs;
    /** The jitter buffer size in millisecond when latest RTP packet received */
    int jitterBufferMs;
    /** The round trip time delay in millisecond when latest RTP packet received */
    int roundTripTimeMs;
}
