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

/**
 * Type to define the NR specific network capabilities for voice over PS including emergency and
 * normal voice calls.
 * @hide
 */
@VintfStability
@JavaDerive(toString=true)
parcelable NrVopsInfo {
    /**
     * Emergency services not supported
     */
    const byte EMC_INDICATOR_NOT_SUPPORTED = 0;
    /**
     * Emergency services supported in NR connected to 5GCN only
     */
    const byte EMC_INDICATOR_NR_CONNECTED_TO_5GCN = 1;
    /**
     * Emergency services supported in E-UTRA connected to 5GCN only
     */
    const byte EMC_INDICATOR_EUTRA_CONNECTED_TO_5GCN = 2;
    /**
     * Emergency services supported in NR connected to 5GCN and E-UTRA connected to 5GCN
     */
    const byte EMC_INDICATOR_BOTH_NR_EUTRA_CONNECTED_TO_5GCN = 3;

    /**
     * Emergency services fallback not supported
     */
    const byte EMF_INDICATOR_NOT_SUPPORTED = 0;
    /**
     * Emergency services fallback supported in NR connected to 5GCN only
     */
    const byte EMF_INDICATOR_NR_CONNECTED_TO_5GCN = 1;
    /**
     * Emergency services fallback supported in E-UTRA connected to 5GCN only
     */
    const byte EMF_INDICATOR_EUTRA_CONNECTED_TO_5GCN = 2;
    /**
     * Emergency services fallback supported in NR connected to 5GCN and E-UTRA connected to 5GCN.
     */
    const byte EMF_INDICATOR_BOTH_NR_EUTRA_CONNECTED_TO_5GCN = 3;

    /**
     * IMS voice over PS session not supported
     */
    const byte VOPS_INDICATOR_VOPS_NOT_SUPPORTED = 0;
    /**
     * IMS voice over PS session supported over 3GPP access
     */
    const byte VOPS_INDICATOR_VOPS_OVER_3GPP = 1;
    /**
     * IMS voice over PS session supported over non-3GPP access
     */
    const byte VOPS_INDICATOR_VOPS_OVER_NON_3GPP = 2;

    /**
     * This indicates if the camped network supports VoNR services, and what kind of services
     * it supports. This information is received from NR network during NR NAS registration
     * procedure through NR REGISTRATION ACCEPT.
     * Refer 3GPP 24.501 EPS 5GS network feature support -> IMS VoPS
     * Values are VOPS_INDICATOR_
     */
    byte vopsSupported;
    /**
     * This indicates if the camped network supports VoNR emergency service. This information
     * is received from NR network through two sources:
     * a. During NR NAS registration procedure through NR REGISTRATION ACCEPT.
     *    Refer 3GPP 24.501 EPS 5GS network feature support -> EMC
     * b. In case the device is not registered on the network.
     *    Refer 3GPP 38.331 SIB1 : ims-EmergencySupport
     *    If device is registered on NR, then this field indicates whether the cell
     *    supports IMS emergency bearer services for UEs in limited service mode.
     * Values are EMC_INDICATOR_
     */
    byte emcSupported;
    /**
     * This indicates if the camped network supports VoNR emergency service fallback. This
     * information is received from NR network during NR NAS registration procedure through
     * NR REGISTRATION ACCEPT.
     * Refer 3GPP 24.501 EPS 5GS network feature support -> EMF
     * Values are EMF_INDICATOR_ from TS 24.501 sec 9.10.3.5.
     */
    byte emfSupported;
}
