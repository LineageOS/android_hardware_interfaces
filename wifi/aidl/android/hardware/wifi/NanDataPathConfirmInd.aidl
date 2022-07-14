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

import android.hardware.wifi.NanDataPathChannelInfo;
import android.hardware.wifi.NanStatus;

/**
 * NAN Data path confirmation indication structure. Event indication is
 * received on both initiator and responder side when negotiation for a
 * data-path finishes on success or failure.
 */
@VintfStability
parcelable NanDataPathConfirmInd {
    /**
     * ID of the data-path.
     */
    int ndpInstanceId;
    /**
     * Indicates whether the data-path setup succeeded (true) or failed (false).
     */
    boolean dataPathSetupSuccess;
    /**
     * MAC address of the peer's data-interface (not its management/discovery interface).
     */
    byte[6] peerNdiMacAddr;
    /**
     * Arbitrary information communicated from the peer as part of the data-path setup process -
     * there is no semantic meaning to these bytes. They are passed-through from sender to receiver
     * as-is with no parsing. Max length: |NanCapabilities.maxAppInfoLen|. NAN Spec: Data Path
     * Attributes / NDP Attribute / NDP Specific Info
     */
    byte[] appInfo;
    /**
     * Failure reason if |dataPathSetupSuccess| is false.
     */
    NanStatus status;
    /**
     * The channel(s) on which the NDP is scheduled to operate.
     * Updates to the operational channels are provided using the |eventDataPathScheduleUpdate|
     * event.
     */
    NanDataPathChannelInfo[] channelInfo;
}
