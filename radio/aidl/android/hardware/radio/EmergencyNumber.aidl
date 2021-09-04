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

import android.hardware.radio.EmergencyNumberSource;
import android.hardware.radio.EmergencyServiceCategory;

/**
 * Emergency number contains information of number, one or more service category(s), zero or more
 * emergency uniform resource names, mobile country code (mcc), mobile network country (mnc) and
 * source(s) that indicate where it comes from.
 *
 * If the emergency number is associated with country, field ‘mcc’ must be provided, otherwise
 * field ‘mcc’ must be an empty string. If the emergency number is associated with network operator,
 * field ‘mcc’ and 'mnc' must be provided, otherwise field ‘mnc’ must be an empty string. If the
 * emergency number is specified with emergency service category(s), field 'categories' must be
 * provided, otherwise field 'categories' must be EmergencyServiceCategories::UNSPECIFIED. If the
 * emergency number is specified with emergency uniform resource names (URN), field 'urns' must be
 * provided, otherwise field 'urns' must be an empty list.
 *
 * A unique EmergencyNumber has a unique combination of ‘number’, ‘mcc’, 'mnc', 'categories' and
 * 'urns' fields. Multiple EmergencyNumberSource should be merged into one 'sources' field via
 * bitwise-OR combination for the same EmergencyNumber.
 *
 * Reference: 3gpp 22.101, Section 10 - Emergency Calls;
 *            3gpp 23.167, Section 6 - Functional description;
 *            3gpp 24.503, Section 5.1.6.8.1 - General;
 *            RFC 5031
 */
@VintfStability
parcelable EmergencyNumber {
    /**
     * The emergency number. The character in the number string should only be the dial pad
     * character('0'-'9', '*', or '#'). For example: 911.
     */
    String number;
    /**
     * 3-digit Mobile Country Code, 0..999. Empty string if not applicable.
     */
    String mcc;
    /**
     * 2 or 3-digit Mobile Network Code, 0..999. Empty string if not applicable.
     */
    String mnc;
    /**
     * The bitfield of EmergencyServiceCategory(s). See EmergencyServiceCategory for the value of
     * each bit.
     */
    EmergencyServiceCategory categories;
    /**
     * The list of emergency Uniform Resource Names (URN).
     */
    String[] urns;
    /**
     * The bitfield of EmergencyNumberSource(s). See EmergencyNumberSource for the value of
     * each bit.
     */
    EmergencyNumberSource sources;
}
