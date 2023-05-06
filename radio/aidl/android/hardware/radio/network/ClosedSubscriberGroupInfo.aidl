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

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable ClosedSubscriberGroupInfo {
    /**
     * Indicates whether the cell is restricted to only CSG members. A cell not broadcasting the
     * CSG Indication but reporting CSG information is considered a Hybrid Cell.
     * Refer to the "csg-Indication" field in 3GPP TS 36.331 section 6.2.2
     * SystemInformationBlockType1.
     * Also refer to "CSG Indicator" in 3GPP TS 25.331 section 10.2.48.8.1 and TS 25.304.
     */
    boolean csgIndication;
    /**
     * The human-readable name of the closed subscriber group operating this cell.
     * Refer to "hnb-Name" in TS 36.331 section 6.2.2 SystemInformationBlockType9.
     * Also refer to "HNB Name" in 3GPP TS25.331 section 10.2.48.8.23 and TS 23.003 section 4.8.
     */
    String homeNodebName;
    /**
     * The identity of the closed subscriber group that the cell belongs to.
     * Refer to "CSG-Identity" in TS 36.336 section 6.3.4.
     * Also refer to "CSG Identity" in 3GPP TS 25.331 section 10.3.2.8 and TS 23.003 section 4.7.
     */
    int csgIdentity;
}
