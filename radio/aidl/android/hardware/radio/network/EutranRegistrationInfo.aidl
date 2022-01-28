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

import android.hardware.radio.network.LteVopsInfo;
import android.hardware.radio.network.NrIndicators;

@VintfStability
parcelable EutranRegistrationInfo {
    /** LTE is attached with eps only. */
    const byte EPS_ONLY = 1;
    /** LTE combined EPS and IMSI attach. */
    const byte COMBINED_EPS_AND_IMSI = 2;
    /** LTE combined attach with CSFB not preferred */
    const byte COMBINED_CSFB_NOT_PREFERRED = 3;
    /** LTE combined attach for SMS only */
    const byte COMBINED_SMS_ONLY = 4;

    /**
     * Network capabilities for voice over PS services. This info is valid only on LTE network and
     * must be present when device is camped on LTE. VopsInfo must be empty when device is camped
     * only on 2G/3G.
     */
    LteVopsInfo lteVopsInfo;
    /**
     * The parameters of NR 5G Non-Standalone. This value is only valid on E-UTRAN, otherwise must
     * be empty.
     */
    NrIndicators nrIndicators;

    /**
     * The type of network attachment. This info is valid only on LTE network and must be present
     * when device has attached to the network.
     */
    byte lteAttachType;

    /** True if emergency attached */
    boolean isEmergencyAttached;
}
