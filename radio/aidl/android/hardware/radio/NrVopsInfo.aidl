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

import android.hardware.radio.EmcIndicator;
import android.hardware.radio.EmfIndicator;
import android.hardware.radio.VopsIndicator;

/**
 * Type to define the NR specific network capabilities for voice over PS including emergency and
 * normal voice calls.
 */
@VintfStability
parcelable NrVopsInfo {
    /**
     * This indicates if the camped network supports VoNR services, and what kind of services
     * it supports. This information is received from NR network during NR NAS registration
     * procedure through NR REGISTRATION ACCEPT.
     * Refer 3GPP 24.501 EPS 5GS network feature support -> IMS VoPS
     */
    VopsIndicator vopsSupported;
    /**
     * This indicates if the camped network supports VoNR emergency service. This information
     * is received from NR network through two sources:
     * a. During NR NAS registration procedure through NR REGISTRATION ACCEPT.
     *    Refer 3GPP 24.501 EPS 5GS network feature support -> EMC
     * b. In case the device is not registered on the network.
     *    Refer 3GPP 38.331 SIB1 : ims-EmergencySupport
     *    If device is registered on NR, then this field indicates whether the cell
     *    supports IMS emergency bearer services for UEs in limited service mode.
     */
    EmcIndicator emcSupported;
    /**
     * This indicates if the camped network supports VoNR emergency service fallback. This
     * information is received from NR network during NR NAS registration procedure through
     * NR REGISTRATION ACCEPT.
     * Refer 3GPP 24.501 EPS 5GS network feature support -> EMF
     */
    EmfIndicator emfSupported;
}
