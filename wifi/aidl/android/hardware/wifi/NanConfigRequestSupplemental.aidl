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
 * Additional NAN configuration request parameters.
 */
@VintfStability
parcelable NanConfigRequestSupplemental {
    /**
     * Specify the Discovery Beacon interval in ms. Specification only applicable if the device
     * transmits Discovery Beacons (based on the Wi-Fi Aware protocol selection criteria). The value
     * can be increased to reduce power consumption (on devices which would transmit Discovery
     * Beacons). However, cluster synchronization time will likely increase.
     * Values are:
     *  - A value of 0 indicates that the HAL sets the interval to a default (implementation
     * specific).
     *  - A positive value.
     */
    int discoveryBeaconIntervalMs;
    /**
     * The number of spatial streams to be used for transmitting NAN management frames (does NOT
     * apply to data-path packets). A small value may reduce power consumption for small discovery
     * packets. Values are:
     *  - A value of 0 indicates that the HAL sets the number to a default (implementation
     * specific).
     *  - A positive value.
     */
    int numberOfSpatialStreamsInDiscovery;
    /**
     * Controls whether the device may terminate listening on a Discovery Window (DW) earlier than
     * the DW termination (16ms) if no information is received. Enabling the feature will result in
     * lower power consumption, but may result in some missed messages and hence increased latency.
     */
    boolean enableDiscoveryWindowEarlyTermination;
    /**
     * Controls whether NAN RTT (ranging) is permitted. Global flag on any NAN RTT operations are
     * allowed. Controls ranging in the context of discovery as well as direct RTT.
     */
    boolean enableRanging;
    /**
     * Controls whether NAN instant communication mode is enabled.
     */
    boolean enableInstantCommunicationMode;
    /**
     * Controls which channel NAN instant communication mode operates on.
     */
    int instantModeChannel;
    /**
     * Controls which cluster to join.
     */
    int clusterId;
}
