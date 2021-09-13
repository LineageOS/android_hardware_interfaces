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

import android.hardware.radio.AccessTechnologySpecificInfo;
import android.hardware.radio.CellIdentity;
import android.hardware.radio.RadioTechnology;
import android.hardware.radio.RegState;
import android.hardware.radio.RegistrationFailCause;

@VintfStability
parcelable RegStateResult {
    /**
     * Registration state. If the RAT is indicated as a GERAN, UTRAN, or CDMA2000 technology, this
     * value reports registration in the Circuit-switched domain. If the RAT is indicated as an
     * EUTRAN, NGRAN, or another technology that does not support circuit-switched services, this
     * value reports registration in the Packet-switched domain.
     */
    RegState regState;
    /**
     * Indicates the available voice radio technology, valid values as defined by RadioTechnology,
     * except LTE_CA, which is no longer a valid value on 1.5 or above. When the device is on
     * carrier aggregation, vendor RIL service should properly report multiple PhysicalChannelConfig
     * elements through IRadio::currentPhysicalChannelConfigs.
     */
    RadioTechnology rat;
    /**
     * Cause code reported by the network in case registration fails. This will be a mobility
     * management cause code defined for MM, GMM, MME or equivalent as appropriate for the RAT.
     */
    RegistrationFailCause reasonForDenial;
    /**
     * CellIdentity
     */
    CellIdentity cellIdentity;
    /**
     * The most-recent PLMN-ID upon which the UE registered (or attempted to register if a failure
     * is reported in the reasonForDenial field). This PLMN shall be in standard format consisting
     * of a 3 digit MCC concatenated with a 2 or 3 digit MNC.
     */
    String registeredPlmn;
    /**
     * Access-technology-specific registration information, such as for CDMA2000.
     */
    AccessTechnologySpecificInfo accessTechnologySpecificInfo;
}
