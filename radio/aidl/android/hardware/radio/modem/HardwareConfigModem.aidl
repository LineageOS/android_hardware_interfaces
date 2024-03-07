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

import android.hardware.radio.RadioTechnology;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable HardwareConfigModem {
    /**
     * RIL attachment model. Values are:
     * 0: single
     * 1: multiple
     * If single, there is a one-to-one relationship between a modem hardware and a ril daemon.
     * If multiple, there is a one-to-many relationship between a modem hardware and several
     * simultaneous ril daemons.
     */
    int rilModel;
    /**
     * Bitset value, based on RadioTechnology.
     */
    RadioTechnology rat;
    /**
     * Maximum number of concurrent active voice calls.
     */
    int maxVoiceCalls;
    /**
     * Maximum number of concurrent active data calls.
     */
    int maxDataCalls;
    /**
     * Maximum number of concurrent standby connections. This is not necessarily an equal sum of the
     * maxVoice and maxData (or a derivative of it) since it really depends on the modem capability,
     * hence it is left for the hardware to define.
     */
    int maxStandby;
}
