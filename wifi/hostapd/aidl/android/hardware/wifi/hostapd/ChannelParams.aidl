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

import android.hardware.wifi.hostapd.BandMask;
import android.hardware.wifi.hostapd.FrequencyRange;

/**
 * Parameters to control the channel selection for the interface.
 */
@VintfStability
parcelable ChannelParams {
    /**
     * Band to use for the SoftAp operations.
     */
    BandMask bandMask;
    /**
     * This option can be used to specify the channel frequencies (in MHz) selected by ACS.
     * If this is an empty list, all channels allowed in selected HW mode
     * are specified implicitly.
     * Note: channels may be overridden by firmware.
     * Note: this option is ignored if ACS is disabled.
     */
    FrequencyRange[] acsChannelFreqRangesMhz;
    /**
     * Whether to enable ACS (Automatic Channel Selection) or not.
     * The channel can be selected automatically at run time by setting
     * this flag, which must enable the ACS survey based algorithm.
     */
    boolean enableAcs;
    /**
     * This option can be used to exclude all DFS channels from the ACS
     * channel list in cases where the driver supports DFS channels.
     **/
    boolean acsShouldExcludeDfs;
    /**
     * Channel number (IEEE 802.11) to use for the interface.
     * If ACS is enabled, this field is ignored.
     *
     * If |enableEdmg| is true, the channel must be set. Refer to
     * P802.11ay_D4.0 29.3.4.
     */
    int channel;
}
