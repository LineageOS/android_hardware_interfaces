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
enum BarringInfoServiceType {
    /**
     * Applicable to UTRAN
     * Barring for all CS services, including registration
     */
    CS_SERVICE,
    /**
     * Barring for all PS services, including registration
     */
    PS_SERVICE,
    /**
     * Barring for mobile-originated circuit-switched voice calls
     */
    CS_VOICE,
    /**
     * Applicable to EUTRAN, NGRAN
     * Barring for mobile-originated signalling for any purpose
     */
    MO_SIGNALLING,
    /**
     * Barring for mobile-originated internet or other interactive data
     */
    MO_DATA,
    /**
     * Barring for circuit-switched fallback calling
     */
    CS_FALLBACK,
    /**
     * Barring for IMS voice calling
     */
    MMTEL_VOICE,
    /**
     * Barring for IMS video calling
     */
    MMTEL_VIDEO,
    /**
     * Applicable to UTRAN, EUTRAN, NGRAN
     * Barring for emergency services, either CS or emergency MMTEL
     */
    EMERGENCY,
    /**
     * Barring for short message services
     */
    SMS,
    /**
     * Operator-specific barring codes; applicable to NGRAN
     */
    OPERATOR_1 = 1001,
    OPERATOR_2 = 1002,
    OPERATOR_3 = 1003,
    OPERATOR_4 = 1004,
    OPERATOR_5 = 1005,
    OPERATOR_6 = 1006,
    OPERATOR_7 = 1007,
    OPERATOR_8 = 1008,
    OPERATOR_9 = 1009,
    OPERATOR_10 = 1010,
    OPERATOR_11 = 1011,
    OPERATOR_12 = 1012,
    OPERATOR_13 = 1013,
    OPERATOR_14 = 1014,
    OPERATOR_15 = 1015,
    OPERATOR_16 = 1016,
    OPERATOR_17 = 1017,
    OPERATOR_18 = 1018,
    OPERATOR_19 = 1019,
    OPERATOR_20 = 1020,
    OPERATOR_21 = 1021,
    OPERATOR_22 = 1022,
    OPERATOR_23 = 1023,
    OPERATOR_24 = 1024,
    OPERATOR_25 = 1025,
    OPERATOR_26 = 1026,
    OPERATOR_27 = 1027,
    OPERATOR_28 = 1028,
    OPERATOR_29 = 1029,
    OPERATOR_30 = 1030,
    OPERATOR_31 = 1031,
    OPERATOR_32 = 1032,
}
