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
@Backing(type="int")
enum MediaDirection {
    /**
     * No RTP/RTCP flow in either direction. The implementation
     * may release the audio resource. Eg. SRVCC.
     */
    NO_FLOW = 0,
    /** Device sends outgoing RTP and drops incoming RTP */
    SEND_ONLY = 1,
    /** Device receives the downlink RTP and does not transmit any uplink RTP */
    RECEIVE_ONLY = 2,
    /** Device sends and receive RTP in both directions */
    SEND_RECEIVE = 3,
    /** No RTP flow however RTCP continues to flow. Eg. HOLD */
    INACTIVE = 4,
}
