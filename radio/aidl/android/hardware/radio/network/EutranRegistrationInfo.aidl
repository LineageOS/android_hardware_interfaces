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

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable EutranRegistrationInfo {
    enum AttachResultType {
        /** Default value. */
        NONE,
        /** LTE is attached with eps only. */
        EPS_ONLY,
        /** LTE combined EPS and IMSI attach. */
        COMBINED,
    }

    /** LTE combined attach with CSFB not preferred */
    const int EXTRA_CSFB_NOT_PREFERRED = 1 << 0;

    /** LTE combined attach for SMS only */
    const int EXTRA_SMS_ONLY = 1 << 1;

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
    AttachResultType lteAttachResultType;

    /** Values are bitwise ORs of EXTRA_* constants */
    int extraInfo;
}
