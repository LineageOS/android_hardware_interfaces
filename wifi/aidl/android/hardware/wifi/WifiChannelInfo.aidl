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

import android.hardware.wifi.WifiChannelWidthInMhz;

/**
 * Channel information.
 */
@VintfStability
parcelable WifiChannelInfo {
    /**
     * Channel width (20, 40, 80, 80+80, 160, 320).
     */
    WifiChannelWidthInMhz width;
    /**
     * Primary 20 MHz channel.
     */
    int centerFreq;
    /**
     * Center frequency (MHz) first segment.
     */
    int centerFreq0;
    /**
     * Center frequency (MHz) second segment.
     */
    int centerFreq1;
}
