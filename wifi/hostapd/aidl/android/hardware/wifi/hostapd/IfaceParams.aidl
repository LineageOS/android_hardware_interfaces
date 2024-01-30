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

import android.hardware.wifi.common.OuiKeyedData;
import android.hardware.wifi.hostapd.ChannelParams;
import android.hardware.wifi.hostapd.HwModeParams;

/**
 * Parameters to use for setting up the dual access point interfaces.
 */
@VintfStability
parcelable IfaceParams {
    /**
     * Name of the interface
     */
    String name;
    /**
     * Additional hardware mode params for the interface
     */
    HwModeParams hwModeParams;
    /**
     * The list of the channel params for the dual interfaces.
     */
    ChannelParams[] channelParams;
    /**
     * Optional vendor-specific configuration parameters.
     */
    @nullable OuiKeyedData[] vendorData;
}
