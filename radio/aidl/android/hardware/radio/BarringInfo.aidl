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

import android.hardware.radio.BarringInfoBarringType;
import android.hardware.radio.BarringInfoBarringTypeSpecificInfo;
import android.hardware.radio.BarringInfoServiceType;

@VintfStability
parcelable BarringInfo {
    /**
     * Combined list of barring services for UTRAN, EUTRAN, and NGRAN.
     *
     * Barring information is defined in:
     * -UTRAN - 3gpp 25.331 Sec 10.2.48.8.6.
     * -EUTRAN - 3gpp 36.331 Sec 6.3.1 SystemInformationBlockType2
     * -NGRAN - 3gpp 38.331 Sec 6.3.2 UAC-BarringInfo and 22.261 Sec 6.22.2.[2-3]
     */
    BarringInfoServiceType serviceType;
    /**
     * The type of barring applied to the service
     */
    BarringInfoBarringType barringType;
    /**
     * Type-specific barring info if applicable
     */
    BarringInfoBarringTypeSpecificInfo barringTypeSpecificInfo;
}
