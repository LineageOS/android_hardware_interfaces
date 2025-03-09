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

package android.hardware.radio.messaging;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable CdmaSmsAddress {
    /**
     * DTMF digits
     * @deprecated Legacy CDMA is unsupported.
     */
    const int DIGIT_MODE_FOUR_BIT = 0;
    /** @deprecated Legacy CDMA is unsupported. */
    const int DIGIT_MODE_EIGHT_BIT = 1;

    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_PLAN_UNKNOWN = 0;
    /**
     * CCITT E.164 and E.163, including ISDN plan
     * @deprecated Legacy CDMA is unsupported.
     */
    const int NUMBER_PLAN_TELEPHONY = 1;
    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_PLAN_RESERVED_2 = 2;
    /**
     * CCITT X.121
     * @deprecated Legacy CDMA is unsupported.
     */
    const int NUMBER_PLAN_DATA = 3;
    /**
     * CCITT F.69
     * @deprecated Legacy CDMA is unsupported.
     */
    const int NUMBER_PLAN_TELEX = 4;
    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_PLAN_RESERVED_5 = 5;
    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_PLAN_RESERVED_6 = 6;
    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_PLAN_RESERVED_7 = 7;
    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_PLAN_RESERVED_8 = 8;
    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_PLAN_PRIVATE = 9;
    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_PLAN_RESERVED_10 = 10;
    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_PLAN_RESERVED_11 = 11;
    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_PLAN_RESERVED_12 = 12;
    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_PLAN_RESERVED_13 = 13;
    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_PLAN_RESERVED_14 = 14;
    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_PLAN_RESERVED_15 = 15;

    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_TYPE_UNKNOWN = 0;
    /**
     * INTERNATIONAL is used when number mode is not data network address. DATA_IP is used when the
     * number mode is data network address.
     * @deprecated Legacy CDMA is unsupported.
     */
    const int NUMBER_TYPE_INTERNATIONAL_OR_DATA_IP = 1;
    /**
     * NATIONAL is used when the number mode is not data netework address. INTERNET_MAIL is used
     * when the number mode is data network address. For INTERNET_MAIL, in the address data
     * "digits", each byte contains an ASCII character. Examples are: "x@y.com,a@b.com"
     * Ref TIA/EIA-637A 3.4.3.3
     * @deprecated Legacy CDMA is unsupported.
     */
    const int NUMBER_TYPE_NATIONAL_OR_INTERNET_MAIL = 2;
    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_TYPE_NETWORK = 3;
    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_TYPE_SUBSCRIBER = 4;
    /**
     * GSM SMS: address value is GSM 7-bit chars
     * @deprecated Legacy CDMA is unsupported.
     */
    const int NUMBER_TYPE_ALPHANUMERIC = 5;
    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_TYPE_ABBREVIATED = 6;
    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_TYPE_RESERVED_7 = 7;

    /**
     * CdmaSmsDigitMode is of two types : 4 bit and 8 bit.
     * For 4-bit type, only "digits" field defined below in this struct is used.
     * Values are DIGIT_MODE_
     * @deprecated Legacy CDMA is unsupported.
     */
    int digitMode;
    /**
     * Used only when digitMode is 8-bit.
     * @deprecated Legacy CDMA is unsupported.
     */
    boolean isNumberModeDataNetwork;
    /**
     * Used only when digitMode is 8-bit. To specify an international address, use the following:
     * digitMode = EIGHT_BIT
     * isNumberModeDataNetwork = true
     * numberType = INTERNATIONAL_OR_DATA_IP
     * numberPlan = TELEPHONY
     * digits = ASCII digits, e.g. '1', '2', '3', '4', and '5'
     * Values are NUMBER_TYPE_
     * @deprecated Legacy CDMA is unsupported.
     */
    int numberType;
    /**
     * Used only when digitMode is 8-bit.
     * Values are NUMBER_PLAN_
     * @deprecated Legacy CDMA is unsupported.
     */
    int numberPlan;
    /**
     * Each byte in this array represents a 4 bit or 8-bit digit of address data.
     * @deprecated Legacy CDMA is unsupported.
     */
    byte[] digits;
}
