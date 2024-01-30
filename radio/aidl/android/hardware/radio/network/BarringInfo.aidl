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

import android.hardware.radio.network.BarringTypeSpecificInfo;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable BarringInfo {
    /**
     * Device is not barred for the given service
     */
    const int BARRING_TYPE_NONE = 0;
    /**
     * Device may be barred based on time and probability factors
     */
    const int BARRING_TYPE_CONDITIONAL = 1;
    /*
     * Device is unconditionally barred
     */
    const int BARRING_TYPE_UNCONDITIONAL = 2;

    /**
     * Applicable to UTRAN
     * Barring for all CS services, including registration
     */
    const int SERVICE_TYPE_CS_SERVICE = 0;
    /**
     * Barring for all PS services, including registration
     */
    const int SERVICE_TYPE_PS_SERVICE = 1;
    /**
     * Barring for mobile-originated circuit-switched voice calls
     */
    const int SERVICE_TYPE_CS_VOICE = 2;
    /**
     * Applicable to EUTRAN, NGRAN
     * Barring for mobile-originated signalling for any purpose
     */
    const int SERVICE_TYPE_MO_SIGNALLING = 3;
    /**
     * Barring for mobile-originated internet or other interactive data
     */
    const int SERVICE_TYPE_MO_DATA = 4;
    /**
     * Barring for circuit-switched fallback calling
     */
    const int SERVICE_TYPE_CS_FALLBACK = 5;
    /**
     * Barring for IMS voice calling
     */
    const int SERVICE_TYPE_MMTEL_VOICE = 6;
    /**
     * Barring for IMS video calling
     */
    const int SERVICE_TYPE_MMTEL_VIDEO = 7;
    /**
     * Applicable to UTRAN, EUTRAN, NGRAN
     * Barring for emergency services, either CS or emergency MMTEL
     */
    const int SERVICE_TYPE_EMERGENCY = 8;
    /**
     * Barring for short message services
     */
    const int SERVICE_TYPE_SMS = 9;
    /**
     * Operator-specific barring codes; applicable to NGRAN
     */
    const int SERVICE_TYPE_OPERATOR_1 = 1001;
    const int SERVICE_TYPE_OPERATOR_2 = 1002;
    const int SERVICE_TYPE_OPERATOR_3 = 1003;
    const int SERVICE_TYPE_OPERATOR_4 = 1004;
    const int SERVICE_TYPE_OPERATOR_5 = 1005;
    const int SERVICE_TYPE_OPERATOR_6 = 1006;
    const int SERVICE_TYPE_OPERATOR_7 = 1007;
    const int SERVICE_TYPE_OPERATOR_8 = 1008;
    const int SERVICE_TYPE_OPERATOR_9 = 1009;
    const int SERVICE_TYPE_OPERATOR_10 = 1010;
    const int SERVICE_TYPE_OPERATOR_11 = 1011;
    const int SERVICE_TYPE_OPERATOR_12 = 1012;
    const int SERVICE_TYPE_OPERATOR_13 = 1013;
    const int SERVICE_TYPE_OPERATOR_14 = 1014;
    const int SERVICE_TYPE_OPERATOR_15 = 1015;
    const int SERVICE_TYPE_OPERATOR_16 = 1016;
    const int SERVICE_TYPE_OPERATOR_17 = 1017;
    const int SERVICE_TYPE_OPERATOR_18 = 1018;
    const int SERVICE_TYPE_OPERATOR_19 = 1019;
    const int SERVICE_TYPE_OPERATOR_20 = 1020;
    const int SERVICE_TYPE_OPERATOR_21 = 1021;
    const int SERVICE_TYPE_OPERATOR_22 = 1022;
    const int SERVICE_TYPE_OPERATOR_23 = 1023;
    const int SERVICE_TYPE_OPERATOR_24 = 1024;
    const int SERVICE_TYPE_OPERATOR_25 = 1025;
    const int SERVICE_TYPE_OPERATOR_26 = 1026;
    const int SERVICE_TYPE_OPERATOR_27 = 1027;
    const int SERVICE_TYPE_OPERATOR_28 = 1028;
    const int SERVICE_TYPE_OPERATOR_29 = 1029;
    const int SERVICE_TYPE_OPERATOR_30 = 1030;
    const int SERVICE_TYPE_OPERATOR_31 = 1031;
    const int SERVICE_TYPE_OPERATOR_32 = 1032;
    /**
     * Combined list of barring services for UTRAN, EUTRAN, and NGRAN.
     *
     * Barring information is defined in:
     * -UTRAN - 3gpp 25.331 Sec 10.2.48.8.6.
     * -EUTRAN - 3gpp 36.331 Sec 6.3.1 SystemInformationBlockType2
     * -NGRAN - 3gpp 38.331 Sec 6.3.2 UAC-BarringInfo and 22.261 Sec 6.22.2.[2-3]
     * Values are SERVICE_TYPE_
     */
    int serviceType;
    /**
     * The type of barring applied to the service
     * Values are BARRING_TYPE_
     */
    int barringType;
    /**
     * Type-specific barring info if applicable
     */
    @nullable BarringTypeSpecificInfo barringTypeSpecificInfo;
}
