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

package android.hardware.radio;

import android.hardware.radio.CellIdentity;
import android.hardware.radio.LteVopsInfo;
import android.hardware.radio.NrIndicators;
import android.hardware.radio.RegState;

@VintfStability
parcelable DataRegStateResult {
    /**
     * Valid reg states are NOT_REG_MT_NOT_SEARCHING_OP, REG_HOME, NOT_REG_MT_SEARCHING_OP,
     * REG_DENIED, UNKNOWN, REG_ROAMING defined in RegState
     */
    RegState regState;
    /**
     * Indicates the available data radio technology, valid values as defined by RadioTechnology.
     */
    int rat;
    /**
     * If registration state is 3 (Registration denied) this is an enumerated reason why
     * registration was denied. See 3GPP TS 24.008, Annex G.6 "Additional cause codes for GMM".
     * 7 == GPRS services not allowed
     * 8 == GPRS services and non-GPRS services not allowed
     * 9 == MS identity cannot be derived by the network
     * 10 == Implicitly detached
     * 14 == GPRS services not allowed in this PLMN
     * 16 == MSC temporarily not reachable
     * 40 == No PDP context activated
     */
    int reasonDataDenied;
    /**
     * The maximum number of simultaneous Data Calls must be established using setupDataCall().
     */
    int maxDataCalls;
    CellIdentity cellIdentity;
    /**
     * Network capabilities for voice over PS services. This info is valid only on LTE network and
     * must be present when device is camped on LTE. vopsInfo must be empty when device is camped
     * only on 2G/3G.
     */
    @nullable LteVopsInfo lteVopsInfo;
    /**
     * The parameters of NR 5G Non-Standalone. This value is only valid on E-UTRAN, otherwise
     * must be empty.
     */
    NrIndicators nrIndicators;
}
