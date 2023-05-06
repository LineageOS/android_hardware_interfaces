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

import android.hardware.radio.network.NgranBands;
import android.hardware.radio.network.OperatorInfo;

/**
 * The CellIdentity structure should be reported once for each element of the PLMN-IdentityInfoList
 * broadcast in SIB1 CellAccessRelatedInfo as per 3GPP TS 38.331 Section 6.3.2.
 * @hide
 */
@VintfStability
@JavaDerive(toString=true)
parcelable CellIdentityNr {
    /**
     * 3-digit Mobile Country Code, in range[0, 999]; This value must be valid for registered or
     *  camped cells; INT_MAX means invalid/unreported.
     */
    String mcc;
    /**
     * 2 or 3-digit Mobile Network Code, in range [0, 999], This value must be valid for
     * registered or camped cells; INT_MAX means invalid/unreported.
     */
    String mnc;
    /**
     * NR Cell Identity in range [0, 68719476735] (36 bits) described in 3GPP TS 38.331, which
     * unambiguously identifies a cell within a PLMN. This value must be valid for registered or
     * camped cells; LONG_MAX (2^63-1) means invalid/unreported.
     */
    long nci;
    /**
     * Physical cell id in range [0, 1007] described in 3GPP TS 38.331. This value must be valid.
     */
    int pci;
    /**
     * 16-bit tracking area code, INT_MAX means invalid/unreported.
     */
    int tac;
    /**
     * NR Absolute Radio Frequency Channel Number, in range [0, 3279165].
     * Reference: 3GPP TS 38.101-1 and 3GPP TS 38.101-2 section 5.4.2.1.
     * This value must be valid.
     */
    int nrarfcn;
    /**
     * OperatorInfo containing alphaLong and alphaShort
     */
    OperatorInfo operatorNames;
    /**
     * Additional PLMN-IDs beyond the primary PLMN broadcast for this cell
     */
    String[] additionalPlmns;
    /**
     * Bands used by the cell.
     */
    NgranBands[] bands;
}
