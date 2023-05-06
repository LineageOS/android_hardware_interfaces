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

/**
 * Directions can be combined to meet various media direction
 * requirements depending on the scenario.
 *
 * Examples:
 *     No Flow      : NO_FLOW - eg. SRVCC.
 *     RTCP-only    : RTCP_TX | RTCP_RX - eg. Local Hold or Dual Hold.
 *     Receive-Only : RTP_RX | RTCP_TX | RTCP_RX - eg. Remote Hold.
 *     Send-Receive : RTP_TX | RTP_RX | RTCP_TX | RTCP_RX - eg. Active call.
 *     Send-Only    : RTP_TX | RTCP_TX | RTCP_RX - eg. Simplex call, voice mail, etc
 * @hide
 */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum MediaDirection {
    /**
     * No RTP/RTCP flow in either direction. The implementation
     * may release the audio resource. Eg. SRVCC.
     */
    NO_FLOW = 0,

    /** Send RTP packets */
    RTP_TX = 1 << 0,

    /** Receive and processes RTP packets */
    RTP_RX = 1 << 1,

    /** Send RTCP packets */
    RTCP_TX = 1 << 2,

    /** Receive RTCP packets */
    RTCP_RX = 1 << 3,
}
