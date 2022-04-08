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

import android.hardware.wifi.WifiDebugHostWakeReasonRxIcmpPacketDetails;
import android.hardware.wifi.WifiDebugHostWakeReasonRxMulticastPacketDetails;
import android.hardware.wifi.WifiDebugHostWakeReasonRxPacketDetails;

/**
 * Structure capturing the count of all the wireless related host wakeup.
 * This is used to capture all the reasons why the host processor
 * (WLAN driver) was woken up by the WLAN firmware.
 * These stats may be used to debug any power issues caused due to frequent
 * wakeup of the host processor by the WLAN firmware.
 */
@VintfStability
parcelable WifiDebugHostWakeReasonStats {
    /**
     * Total count of cmd/event wakes.
     * These must account for all wakeups due to WLAN management
     * commands/events received over the air.
     */
    int totalCmdEventWakeCnt;
    /**
     * Vector of wake counts per cmd/event type.
     * The number of command types and their meaning is only understood by the
     * vendor.
     */
    int[] cmdEventWakeCntPerType;
    /**
     * Total count of driver/firmware wakes.
     * These must account for all wakeups due to local driver/firmware
     * interactions. These include all vendor implementation specific
     * interactions like any heart-beat monitoring, bus management, etc.
     */
    int totalDriverFwLocalWakeCnt;
    /**
     * Vector of wake counts per driver/firmware interaction type.
     * The number of command types and their meaning is only understood by the
     * vendor.
     */
    int[] driverFwLocalWakeCntPerType;
    /**
     * Total data rx packets that woke up host.
     */
    int totalRxPacketWakeCnt;
    WifiDebugHostWakeReasonRxPacketDetails rxPktWakeDetails;
    WifiDebugHostWakeReasonRxMulticastPacketDetails rxMulticastPkWakeDetails;
    WifiDebugHostWakeReasonRxIcmpPacketDetails rxIcmpPkWakeDetails;
}
