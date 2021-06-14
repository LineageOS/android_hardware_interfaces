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

import android.hardware.radio.CdmaSmsDigitMode;
import android.hardware.radio.CdmaSmsNumberMode;
import android.hardware.radio.CdmaSmsNumberPlan;
import android.hardware.radio.CdmaSmsNumberType;

@VintfStability
parcelable CdmaSmsAddress {
    /**
     * CdmaSmsDigitMode is of two types : 4 bit and 8 bit.
     * For 4-bit type, only "digits" field defined below in this struct is used.
     */
    CdmaSmsDigitMode digitMode;
    /**
     * Used only when digitMode is 8-bit.
     */
    CdmaSmsNumberMode numberMode;
    /**
     * Used only when digitMode is 8-bit. To specify an international address, use the following:
     * digitMode = CdmaSmsDigitMode:EIGHT_BIT:
     * numberMode = CdmaSmsNumberMode:NOT_DATA_NETWORK
     * numberType = CdmaSmsNumberType:INTERNATIONAL_OR_DATA_IP
     * numberPlan = CdmaSmsNumberPlan:TELEPHONY
     * numberOfDigits = number of digits
     * digits = ASCII digits, e.g. '1', '2', '3', '4', and '5'
     */
    CdmaSmsNumberType numberType;
    /**
     * Used only when digitMode is 8-bit.
     */
    CdmaSmsNumberPlan numberPlan;
    /**
     * Each byte in this array represents a 4 bit or 8-bit digit of address data.
     */
    byte[] digits;
}
