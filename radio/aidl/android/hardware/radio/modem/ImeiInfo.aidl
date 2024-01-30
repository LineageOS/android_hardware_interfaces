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

package android.hardware.radio.modem;

/**
 * ImeiInfo to encapsulate the IMEI information from modem
 * @hide
 */
@VintfStability
@JavaDerive(toString=true)
parcelable ImeiInfo {
    @VintfStability
    @Backing(type="int")
    /**
     * ImeiType enum is used identify the IMEI as primary or secondary as mentioned in GSMA TS.37
     */
    enum ImeiType {
        /**
         * This is the primary IMEI of the device as mentioned in the GSMA TS.37. In a multi-SIM
         * device the modem must set one IMEI with this type as mentioned in GSMA TS37_2.2_REQ_8.
         * A single SIM with one IMEI must by default set that IMEI with this type.
         */
        PRIMARY = 1,
        /** This is not the primary IMEI of the device */
        SECONDARY = 2,
    }

    /** Primary or secondary IMEI as mentioned in GSMA spec TS.37 */
    ImeiType type;
    /**
     * IMEI value, see 3gpp spec 23.003 section 6. Note: This primary IMEI mapping must be
     * permanent throughout the lifetime of the device irrespective of the factory data reset,
     * SIM activations or swaps.
     */
    String imei;
    /**
     * IMEI software version, see 3gpp spec 23.003 section 6.
     */
    String svn;
}
