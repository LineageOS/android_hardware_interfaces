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

package android.hardware.radio.sim;

import android.hardware.radio.sim.PersoSubstate;
import android.hardware.radio.sim.PinState;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable AppStatus {
    const int APP_STATE_UNKNOWN = 0;
    const int APP_STATE_DETECTED = 1;
    /**
     * If PIN1 or UPin is required
     */
    const int APP_STATE_PIN = 2;
    /**
     * If PUK1 or Puk for Upin is required
     */
    const int APP_STATE_PUK = 3;
    /**
     * perso_substate must be looked at when app_state is assigned to this value
     */
    const int APP_STATE_SUBSCRIPTION_PERSO = 4;
    const int APP_STATE_READY = 5;

    const int APP_TYPE_UNKNOWN = 0;
    const int APP_TYPE_SIM = 1;
    const int APP_TYPE_USIM = 2;
    const int APP_TYPE_RUIM = 3;
    const int APP_TYPE_CSIM = 4;
    const int APP_TYPE_ISIM = 5;

    /**
     * Values are APP_TYPE_
     */
    int appType;
    /**
     * Values are APP_STATE_
     */
    int appState;
    /**
     * Applicable only if appState == SUBSCRIPTION_PERSO
     */
    PersoSubstate persoSubstate;
    /**
     * e.g., from 0xA0, 0x00 -> 0x41, 0x30, 0x30, 0x30
     */
    String aidPtr;
    String appLabelPtr;
    /**
     * Applicable to USIM, CSIM and ISIM
     */
    boolean pin1Replaced;
    PinState pin1;
    PinState pin2;
}
