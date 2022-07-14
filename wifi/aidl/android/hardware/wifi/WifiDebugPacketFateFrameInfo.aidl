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

package android.hardware.wifi;

import android.hardware.wifi.WifiDebugPacketFateFrameType;

/**
 * Information regarding the frame transmitted/received.
 */
@VintfStability
parcelable WifiDebugPacketFateFrameInfo {
    /**
     * The type of MAC-layer frame that this frame_info holds.
     * - For data frames, use FRAME_TYPE_ETHERNET_II.
     * - For management frames, use FRAME_TYPE_80211_MGMT.
     * - If the type of the frame is unknown, use FRAME_TYPE_UNKNOWN.
     */
    WifiDebugPacketFateFrameType frameType;
    /**
     * The number of bytes included in |frameContent|.
     * If the frame contents are missing (e.g. RX frame dropped in firmware),
     * |frameLen| must be set to 0.
     */
    long frameLen;
    /**
     * Host clock when this frame was received by the driver (either outbound
     * from the host network stack, or inbound from the firmware).
     * - The timestamp must be taken from a clock which includes time the host
     *   spent suspended (e.g. ktime_get_boottime()).
     * - If no host timestamp is available (e.g. RX frame was dropped in firmware),
     *   this field must be set to 0.
     */
    long driverTimestampUsec;
    /**
     * Firmware clock when this frame was received by the firmware
     * (either outbound from the host, or inbound from a remote  station).
     * - The timestamp must be taken from a clock which includes time firmware
     *   spent suspended (if applicable).
     * - If no firmware timestamp is available (e.g. TX frame was dropped by the
     *   driver), then this field must be set to 0.
     * - Consumers of |frameInfo| must not assume any synchronization between
     *   the driver and firmware clocks.
     */
    long firmwareTimestampUsec;
    /**
     * Actual frame content. This is the raw bytes of the corresponding packet.
     * - Should be provided for TX frames originated by the host.
     * - Should be provided for RX frames received by the driver.
     * - Optionally provided for TX frames originated by firmware.
     *   (At discretion of HAL implementation.)
     * - Optionally provided for RX frames dropped in firmware.
     *   (At discretion of HAL implementation.)
     * - If frame content is not provided, |frameLen| must be set to 0.
     */
    byte[] frameContent;
}
