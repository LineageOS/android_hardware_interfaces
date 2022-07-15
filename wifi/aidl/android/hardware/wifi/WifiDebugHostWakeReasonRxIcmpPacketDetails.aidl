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

/**
 * Struct capturing the count of all rx ICMP packets that caused
 * host wakeup.
 */
@VintfStability
parcelable WifiDebugHostWakeReasonRxIcmpPacketDetails {
    /**
     * Wake icmp packet count.
     */
    int icmpPkt;
    /**
     * Wake icmp6 packet count.
     */
    int icmp6Pkt;
    /**
     * Wake icmp6 RA packet count.
     */
    int icmp6Ra;
    /**
     * Wake icmp6 NA packet count.
     */
    int icmp6Na;
    /**
     * Wake icmp6 NS packet count.
     */
    int icmp6Ns;
}
