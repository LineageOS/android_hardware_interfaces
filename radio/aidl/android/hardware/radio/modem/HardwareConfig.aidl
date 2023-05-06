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

package android.hardware.radio.modem;

import android.hardware.radio.modem.HardwareConfigModem;
import android.hardware.radio.modem.HardwareConfigSim;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable HardwareConfig {
    const int STATE_ENABLED = 0;
    const int STATE_STANDBY = 1;
    const int STATE_DISABLED = 2;

    const int TYPE_MODEM = 0;
    const int TYPE_SIM = 1;

    /**
     * Values are TYPE_
     */
    int type;
    /**
     * RadioConst:MAX_UUID_LENGTH is max length of the string
     */
    String uuid;
    /**
     * Values are STATE_
     */
    int state;
    /**
     * Valid only if type is Modem and size = 1 else must be empty. Only one of modem or sim must
     * have size = 1 based on the HardwareConfigType, and the other must have size = 0.
     */
    HardwareConfigModem[] modem;
    /**
     * Valid only if type is SIM and size = 1 else must be empty. Only one of modem or sim must
     * have size = 1 based on the HardwareConfigType, and the other must have size = 0.
     */
    HardwareConfigSim[] sim;
}
