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

import android.hardware.wifi.WifiAntennaMode;
import android.hardware.wifi.WifiBand;

/**
 * Wifi radio configuration.
 */
@VintfStability
parcelable WifiRadioConfiguration {
    /**
     * Band on which this radio chain is operating.
     * Valid values of bandInfo are: BAND_24GHZ, BAND_5GHZ, BAND_6GHZ and
     * BAND_60GHZ.
     *
     */
    WifiBand bandInfo;
    /**
     * Wifi Antenna configuration.
     */
    WifiAntennaMode antennaMode;
}
