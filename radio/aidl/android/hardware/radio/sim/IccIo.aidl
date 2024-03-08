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

package android.hardware.radio.sim;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable IccIo {
    /**
     * One of the commands listed for TS 27.007 +CRSM
     */
    int command;
    /**
     * EF ID
     */
    int fileId;
    /**
     * "pathid" from TS 27.007 +CRSM command. Path is in hex ASCII format eg "7f205f70"
     * Path must always be provided.
     */
    String path;
    /**
     * Value of p1 defined as per 3GPP TS 51.011
     */
    int p1;
    /**
     * Value of p2 defined as per 3GPP TS 51.011
     */
    int p2;
    /**
     * Value of p3 defined as per 3GPP TS 51.011
     */
    int p3;
    /**
     * Information to be written to the SIM
     */
    String data;
    String pin2;
    /**
     * AID value, See ETSI 102.221 8.1 and 101.220 4, empty string if no value.
     */
    String aid;
}
