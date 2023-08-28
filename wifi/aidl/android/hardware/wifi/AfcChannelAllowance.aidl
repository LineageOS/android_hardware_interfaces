/*
 * Copyright (C) 2023 The Android Open Source Project
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

import android.hardware.wifi.AvailableAfcChannelInfo;
import android.hardware.wifi.AvailableAfcFrequencyInfo;

/**
 * Defines the maximum permissible power spectral density to support 6Ghz with standard power for
 * AFC. The maximum power can be either defined based on frequencies or channel number.
 *
 * Note, based on AFC server support, either availableAfcFrequencyInfos or availableAfcChannelInfos
 * may be empty. If one of them is empty while the other is not, use the non-empty one and ignore
 * the empty one. If both are empty then it means 6Ghz standard power should not be supported at
 * all.
 *
 * If availableAfcFrequencyInfos is non-empty, set the max permissible power according to the maxPsd
 * per frequency range, and disallow emmision on 6Ghz frequencies not included in the structure.
 *
 * If availableAfcChannelInfos is non-empty, set the max permissible power according to the
 * maxEirpDbm per channel, and disallow emmision on 6Ghz channels not included in the structure.
 */
@VintfStability
parcelable AfcChannelAllowance {
    /**
     * AFC max permissible information queried from AFC server based on frequency.
     */
    AvailableAfcFrequencyInfo[] availableAfcFrequencyInfos;
    /**
     * AFC max permissible information queried from AFC server on channel number.
     */
    AvailableAfcChannelInfo[] availableAfcChannelInfos;
    /**
     * The time in UTC at which this information expires, as the difference, measured in
     * milliseconds between the expiration time and midnight, January 1, 1970 UTC.
     */
    long availabilityExpireTimeMs;
}
