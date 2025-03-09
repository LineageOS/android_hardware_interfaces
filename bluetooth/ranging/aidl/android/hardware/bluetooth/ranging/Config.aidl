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

import android.hardware.bluetooth.ranging.Ch3cShapeType;
import android.hardware.bluetooth.ranging.ChannelSelectionType;
import android.hardware.bluetooth.ranging.CsSyncPhyType;
import android.hardware.bluetooth.ranging.ModeType;
import android.hardware.bluetooth.ranging.Role;
import android.hardware.bluetooth.ranging.RttType;
import android.hardware.bluetooth.ranging.SubModeType;

/**
 * LE CS Config Complete data of Channel Sounding.
 * See BLUETOOTH CORE SPECIFICATION Version 6.0 | Vol 4, Part E 7.8.137 for details.
 *
 * Specification: https://www.bluetooth.com/specifications/specs/core60-html/
 */
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
    /**
     * Minimum number of CS main mode steps to be executed before a submode step is executed
     * Value: 0x02 to 0xFF
     */
    int minMainModeSteps;
    /**
     * Maximum number of CS main mode steps to be executed before a submode step is executed
     * Value: 0x02 to 0xFF
     */
    int maxMainModeSteps;
    /**
     * Number of main mode steps taken from the end of the last CS subevent to be repeated at
     * the beginning of the current CS subevent directly after the last mode-0 step of that event
     * Value: 0x00 to 0x03
     */
    byte mainModeRepetition;
    /**
     * Number of CS mode-0 steps to be included at the beginning of each CS subevent
     * Value: 0x00 to 0x03
     */
    byte mode0Steps;
    /**
     * The Channel Sounding role of the local device
     */
    Role role;
    /**
     * Indicates the PHY to be used for CS_SYNC exchanges during the CS procedure
     */
    CsSyncPhyType csSyncPhyType;
    /**
     * Indicates the Channel Selection Algorithm to be used during the CS procedure for non-mode-0
     * steps
     */
    ChannelSelectionType channelSelectionType;
    /**
     * Indicates the selected shape
     */
    Ch3cShapeType ch3cShapeType;
    /**
     * Number of channels skipped in each rising and falling sequence
     * Value: 0x02 to 0x08
     */
    byte ch3cJump;
    /**
     * The number of times the map represented by the Channel_Map field is to be cycled through
     * for non-mode-0 steps within a CS procedure
     * Value: 0x01 to 0xFF
     */
    int channelMapRepetition;
    /**
     * Interlude time in microseconds between the RTT packets
     * Value: 0x0A, 0x14, 0x1E, 0x28, 0x32, 0x3C, 0x50, or 0x91
     * unit: us
     */
    int tIp1TimeUs;
    /**
     * Interlude time in microseconds between the CS tones
     * Value: 0x0A, 0x14, 0x1E, 0x28, 0x32, 0x3C, 0x50, or 0x91
     * unit: us
     */
    int tIp2TimeUs;
    /**
     * Time in microseconds for frequency changes
     * Value: 0x0F, 0x14, 0x1E, 0x28, 0x32, 0x3C, 0x50, 0x64, 0x78, or 0x96
     * unit: us
     */
    int tFcsTimeUs;
    /**
     * Time in microseconds for the phase measurement period of the CS tones
     * Value: 0x0A, 0x14, or 0x28
     * unit: us
     */
    byte tPmTimeUs;
    /**
     * Time in microseconds for the antenna switch period of the CS tones supported by local device
     * Value: 0, 1, 2, 4, 10 us
     * see BLUETOOTH CORE SPECIFICATION Version 6.0 | Vol 4, Part E 7.8.130
     */
    byte tSwTimeUsSupportedByLocal;
    /**
     * Time in microseconds for the antenna switch period of the CS tones supported by remote device
     * Value: 0, 1, 2, 4, 10 us
     * see BLUETOOTH CORE SPECIFICATION Version 6.0 | Vol 4, Part E 7.7.65.39
     */
    byte tSwTimeUsSupportedByRemote;
    const int BLE_CONN_INTERVAL_UNAVAILABLE = 0;
    /**
     * BLE event connection interval, a multiple of 1.25 ms in the range 7.5 ms to 4.0 s
     * see BLUETOOTH CORE SPECIFICATION Version 6.0 | Vol 6, Part B 4.5.1
     * Unit: 1.25ms
     */
    int bleConnInterval = BLE_CONN_INTERVAL_UNAVAILABLE;
}
