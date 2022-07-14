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
 * Packet stats for different traffic categories.
 */
@VintfStability
parcelable StaLinkLayerIfacePacketStats {
    /**
     * Number of received unicast data packets.
     */
    long rxMpdu;
    /**
     * Number of successfully transmitted unicast data pkts (ACK rcvd).
     */
    long txMpdu;
    /**
     * Number of transmitted unicast data pkt losses (no ACK).
     */
    long lostMpdu;
    /**
     * Number of transmitted unicast data retry pkts.
     */
    long retries;
}
