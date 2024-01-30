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

package android.hardware.radio.network;

import android.hardware.radio.network.ClosedSubscriberGroupInfo;
import android.hardware.radio.network.EutranBands;
import android.hardware.radio.network.OperatorInfo;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable CellIdentityLte {
    /**
     * 3-digit Mobile Country Code, 0..999, empty string if unknown
     */
    String mcc;
    /**
     * 2 or 3-digit Mobile Network Code, 0..999, empty string if unknown
     */
    String mnc;
    /**
     * 28-bit Cell Identity described in TS TS 27.007, INT_MAX if unknown
     */
    int ci;
    /**
     * Physical cell id 0..503; this value must be valid
     */
    int pci;
    /**
     * 16-bit tracking area code, INT_MAX if unknown
     */
    int tac;
    /**
     * 18-bit LTE Absolute RF Channel Number; this value must be valid
     */
    int earfcn;
    /**
     * OperatorInfo containing alphaLong and alphaShort
     */
    OperatorInfo operatorNames;
    /**
     * Cell bandwidth, in kHz.
     */
    int bandwidth;
    /**
     * Additional PLMN-IDs beyond the primary PLMN broadcast for this cell
     */
    String[] additionalPlmns;
    /**
     * Information about any closed subscriber group ID for this cell
     */
    @nullable ClosedSubscriberGroupInfo csgInfo;
    /**
     * Bands used by the cell.
     */
    EutranBands[] bands;
}
