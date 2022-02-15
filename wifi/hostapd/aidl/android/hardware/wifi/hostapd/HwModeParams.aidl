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

package android.hardware.wifi.hostapd;

import android.hardware.wifi.hostapd.ChannelBandwidth;

/**
 * Parameters to control the HW mode for the interface.
 */
@VintfStability
parcelable HwModeParams {
    /**
     * Whether IEEE 802.11n (HT) is enabled or not.
     * Note: hwMode=G (2.4 GHz) and hwMode=A (5 GHz) is used to specify
     * the band.
     */
    boolean enable80211N;
    /**
     * Whether IEEE 802.11ac (VHT) is enabled or not.
     * Note: hw_mode=a is used to specify that 5 GHz band is used with VHT.
     */
    boolean enable80211AC;
    /**
     * Whether IEEE 802.11ax (High Efficiency) is enabled or not.
     * Note: hw_mode=a is used to specify that 5 GHz band or 6 GHz band is
     * used with High Efficiency.
     */
    boolean enable80211AX;
    /**
     * Whether 6GHz band enabled or not on softAp.
     * Note: hw_mode=a is used to specify that 5 GHz band or 6 GHz band is
     * used.
     */
    boolean enable6GhzBand;
    /**
     * Whether High Efficiency single user beamformer in enabled or not on softAp.
     * Note: this is only applicable if 802.11ax is supported for softAp
     */
    boolean enableHeSingleUserBeamformer;
    /**
     * Whether High Efficiency single user beamformee is enabled or not on softAp.
     * Note: this is only applicable if 802.11ax is supported for softAp
     */
    boolean enableHeSingleUserBeamformee;
    /**
     * Whether High Efficiency multiple user beamformer is enabled or not on softAp.
     * Note: this is only applicable if 802.11ax is supported for softAp
     */
    boolean enableHeMultiUserBeamformer;
    /**
     * Whether High Efficiency Target Wait Time (TWT) is enabled or not on softAp.
     * Note: this is only applicable if 802.11ax is supported for softAp
     */
    boolean enableHeTargetWakeTime;
    /**
     * Enable EDMG (802.11ay), this option is only allowed for the 60GHz band.
     */
    boolean enableEdmg;
    /**
     * Whether IEEE 802.11be (Extreme High Throughput) is enabled or not.
     * Note: hw_mode=a is used to specify that 5 GHz band or 6 GHz band is
     * used with Extreme High Throughput.
     */
    boolean enable80211BE;
    /**
     * Limit on maximum channel bandwidth for the softAp.
     * For automatic selection with no limit use BANDWIDTH_AUTO
     */
    ChannelBandwidth maximumChannelBandwidth;
}
