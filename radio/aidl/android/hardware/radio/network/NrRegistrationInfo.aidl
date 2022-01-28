/*
 * Copyright (C) 2022 The Android Open Source Project
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

import android.hardware.radio.network.NrVopsInfo;

@VintfStability
parcelable NrRegistrationInfo {
    /** 5GS registration result value - TS 24.501 9.11.3.6 */
    const byte REGISTERED_OVER_3GPP = 1;
    const byte REGISTERED_OVER_NON_3GPP = 2;
    const byte REGISTERED_OVER_3GPP_AND_NON_3GPP = 3;

    /**
     * Network capabilities for voice over PS services. This info is valid only on NR network and
     * must be present when the device is camped on NR. NrVopsInfo must be empty when the device is
     * not camped on NR.
     */
    NrVopsInfo ngranNrVopsInfo;
    /** 5GS registration result value - TS 24.501 9.11.3.6 */
    byte resultValue;
    /** 5GS registration result SMS over NAS - TS 24.501 9.11.3.6 */
    boolean isSmsOverNasAllowed;
    /** True if emergency registered */
    boolean isEmergencyRegistered;
}
