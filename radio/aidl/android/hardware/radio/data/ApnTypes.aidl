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

package android.hardware.radio.data;

/** @hide */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum ApnTypes {
    /**
     * None
     */
    NONE = 0,
    /**
     * APN type for default data traffic
     */
    DEFAULT = 1 << 0,
    /**
     * APN type for MMS traffic
     */
    MMS = 1 << 1,
    /**
     * APN type for SUPL assisted GPS
     */
    SUPL = 1 << 2,
    /**
     * APN type for DUN traffic
     */
    DUN = 1 << 3,
    /**
     * APN type for HiPri traffic
     */
    HIPRI = 1 << 4,
    /**
     * APN type for FOTA
     */
    FOTA = 1 << 5,
    /**
     * APN type for IMS
     */
    IMS = 1 << 6,
    /**
     * APN type for CBS
     */
    CBS = 1 << 7,
    /**
     * APN type for IA Initial Attach APN
     */
    IA = 1 << 8,
    /**
     * APN type for Emergency PDN. This is not an IA apn, but is used for access to carrier services
     * in an emergency call situation.
     */
    EMERGENCY = 1 << 9,
    /**
     * APN type for Mission Critical Service
     * Reference: 3GPP TS 22.280 V15.3.0
     */
    MCX = 1 << 10,
    /**
     * APN type for XCAP
     */
    XCAP = 1 << 11,
    /**
     * APN type for VSIM.
     */
    VSIM = 1 << 12,
    /**
     * APN type for BIP.
     */
    BIP = 1 << 13,
    /**
     * APN type for ENTERPRISE
     */
    ENTERPRISE = 1 << 14,
    /**
     * APN type for RCS (Rich Communication Services)
     */
    RCS = 1 << 15
}
