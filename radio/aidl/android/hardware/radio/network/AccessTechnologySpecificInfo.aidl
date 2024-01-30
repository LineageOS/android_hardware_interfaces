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

import android.hardware.radio.network.Cdma2000RegistrationInfo;
import android.hardware.radio.network.EutranRegistrationInfo;
import android.hardware.radio.network.NrVopsInfo;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
union AccessTechnologySpecificInfo {
    boolean noinit;
    Cdma2000RegistrationInfo cdmaInfo;
    EutranRegistrationInfo eutranInfo;
    /**
     * Network capabilities for voice over PS services. This info is valid only on NR network and
     * must be present when the device is camped on NR. NrVopsInfo must be empty when the device is
     * not camped on NR.
     */
    NrVopsInfo ngranNrVopsInfo;
    /**
     * True if the dual transfer mode is supported. Refer to 3GPP TS 44.108 section 3.4.25.3
     */
    boolean geranDtmSupported;
}
