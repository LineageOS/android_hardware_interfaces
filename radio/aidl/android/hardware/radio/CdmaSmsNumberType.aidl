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

@VintfStability
@Backing(type="int")
enum CdmaSmsNumberType {
    UNKNOWN,
    /**
     * INTERNATIONAL is used when number mode is not data network address. DATA_IP is used when the
     * number mode is data network address.
     */
    INTERNATIONAL_OR_DATA_IP,
    /**
     * NATIONAL is used when the number mode is not data netework address. INTERNET_MAIL is used
     * when the number mode is data network address. For INTERNET_MAIL, in the address data
     * "digits", each byte contains an ASCII character. Examples are: "x@y.com,a@b.com"
     * Ref TIA/EIA-637A 3.4.3.3
     */
    NATIONAL_OR_INTERNET_MAIL,
    NETWORK,
    SUBSCRIBER,
    /**
     * GSM SMS: address value is GSM 7-bit chars
     */
    ALPHANUMERIC,
    ABBREVIATED,
    RESERVED_7,
}
