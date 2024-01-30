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
 * RTP Control Protocol Extended Reports (RTCP XR) Blocks, See RFC 3611 section 4
 * @hide
 */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum RtcpXrReportBlockType {
    /** Disable RTCP XR */
    RTCPXR_NONE = 0,
    /** Loss RLE Report Block */
    RTCPXR_LOSS_RLE_REPORT_BLOCK = 1 << 0,
    /** Duplicate RLE Report Block */
    RTCPXR_DUPLICATE_RLE_REPORT_BLOCK = 1 << 1,
    /** Packet Receipt Times Report Block */
    RTCPXR_PACKET_RECEIPT_TIMES_REPORT_BLOCK = 1 << 2,
    /** Receiver Reference Time Report Block */
    RTCPXR_RECEIVER_REFERENCE_TIME_REPORT_BLOCK = 1 << 3,
    /** DLRR Report Block */
    RTCPXR_DLRR_REPORT_BLOCK = 1 << 4,
    /** Statistics Summary Report Block */
    RTCPXR_STATISTICS_SUMMARY_REPORT_BLOCK = 1 << 5,
    /** VoIP Metrics Report Block */
    RTCPXR_VOIP_METRICS_REPORT_BLOCK = 1 << 6,
}
