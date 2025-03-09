/*
 * Copyright 2024 The Android Open Source Project
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

import android.hardware.bluetooth.ranging.ProcedureAbortReason;
import android.hardware.bluetooth.ranging.SubeventResultData;

/**
 * The measured data for a whole procedure, it includes all local and remote related data.
 */
@VintfStability
parcelable ChannelSoundingProcedureData {
    /**
     * CS procedure count since completion of the Channel Sounding Security Start procedure
     */
    int procedureCounter;
    /**
     * The procequre sequence since completion of the Channel Sounding Procecedure Enable procedure,
     * this is not defined by spec, BT
     */
    int procedureSequence;
    const byte SELECTED_TX_POWER_UNAVAILABLE = 0x7Fu8;
    /**
     * Transmit power level used for CS procedure of initiator.
     * See BLUETOOTH CORE SPECIFICATION Version 6.0 | Vol 4, Part E 7.7.65.43
     * ** HCI_LE_CS_Procedure_Enable_Complete#selected_tx_power
     * See BLUETOOTH Ranging Service Version 1.0 3.2.1.2
     * ** Ranging Header#Selected TX power
     * Range: -127 to 20
     * Unit: dBm
     * value: 0x7F - Transmit power level is unavailable
     */
    byte initiatorSelectedTxPower = SELECTED_TX_POWER_UNAVAILABLE;
    /**
     * Transmit power level used for CS procedure of reflector.
     * See BLUETOOTH CORE SPECIFICATION Version 6.0 | Vol 4, Part E 7.7.65.43
     * ** HCI_LE_CS_Procedure_Enable_Complete#selected_tx_power
     * See BLUETOOTH Ranging Service Version 1.0 3.2.1.2
     * ** Ranging Header#Selected TX power
     * Range: -127 to 20
     * Unit: dBm
     * value: 0x7F - Transmit power level is unavailable
     */
    byte reflectorSelectedTxPower = SELECTED_TX_POWER_UNAVAILABLE;
    /**
     * The subevent result data of initiator
     */
    SubeventResultData[] initiatorSubeventResultData;
    /**
     * Indicates the procedure abort reason of the initiator
     */
    ProcedureAbortReason initiatorProcedureAbortReason;
    /**
     * The subevent result data of reflector
     */
    SubeventResultData[] reflectorSubeventResultData;
    /**
     * Indicates the procedure abort reason of the initiator
     */
    ProcedureAbortReason reflectorProcedureAbortReason;
}
