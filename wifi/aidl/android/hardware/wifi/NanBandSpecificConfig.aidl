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
 * NAN band-specific configuration.
 */
@VintfStability
parcelable NanBandSpecificConfig {
    /**
     * RSSI values controlling clustering behavior per spec. RSSI values are specified without a
     * sign, e.g. a value of -65dBm would be specified as 65.
     */
    byte rssiClose;
    byte rssiMiddle;
    /**
     * RSSI value determining whether discovery is near (used if enabled in discovery by
     * |NanDiscoveryCommonConfig.useRssiThreshold|).
     * RSSI values are specified without a sign, e.g. a value of -65dBm would be specified as 65.
     * NAN Spec: RSSI_close_proximity
     */
    byte rssiCloseProximity;
    /**
     * Dwell time of each discovery channel in milliseconds. If set to 0, then the firmware
     * determines the dwell time to use.
     */
    char dwellTimeMs;
    /**
     * Scan period of each discovery channel in seconds. If set to 0, then the firmware determines
     * the scan period to use.
     */
    char scanPeriodSec;
    /**
     * Specifies the discovery window interval for Sync beacons and SDF's.
     * Valid values of DW Interval are: 1, 2, 3, 4 and 5 corresponding to 1, 2, 4, 8, and 16 DWs.
     * Value of 0:
     *  - reserved in 2.4GHz band
     *  - no wakeup at all in 5GHz band
     * The publish/subscribe period values don't override the device level configurations if
     * they are specified.
     * Configuration is only used only if |validDiscoveryWindowIntervalVal| is set to true.
     * NAN Spec: Device Capability Attribute / 2.4 GHz DW, Device Capability Attribute / 5 GHz DW
     */
    boolean validDiscoveryWindowIntervalVal;
    byte discoveryWindowIntervalVal;
}
