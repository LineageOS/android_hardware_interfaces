/*
 * Copyright 2023 The Android Open Source Project
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

package android.hardware.bluetooth.ranging;

import android.hardware.bluetooth.ranging.ModeType;
import android.hardware.bluetooth.ranging.RttType;
import android.hardware.bluetooth.ranging.SubModeType;

@VintfStability
parcelable Config {
    /**
     * Main_Mode_Type of the CS conifg
     */
    ModeType modeType;
    /**
     * Sub_Mode_Type of the CS conifg
     */
    SubModeType subModeType;
    /**
     * RTT_Type of the CS conifg
     */
    RttType rttType;
    /**
     * Channel_Map of the CS conifg, this parameter contains 80 1-bit fields. The nth such field
     * (in the range 0 to 78) contains the value for the CS channel index n.
     *
     * Channel n is enabled for CS procedure = 1
     * Channel n is disabled for CS procedure = 0
     */
    byte[10] channelMap;
}
