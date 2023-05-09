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

package android.hardware.radio.config;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable SimPortInfo {
    /**
     * Integrated Circuit Card IDentifier (ICCID) is unique identifier of the SIM card. File is
     * located in the SIM card at EFiccid (0x2FE2) as per ETSI 102.221. The ICCID is defined by
     * the ITU-T recommendation E.118 ISO/IEC 7816.
     *
     * This data is applicable only when cardState is CardStatus.STATE_PRESENT.
     *
     * This is the ICCID of the currently enabled profile. If no profile is enabled,
     * then it will contain the default boot profile’s ICCID.
     * If the EFiccid does not exist in the default boot profile, it will be null.
     */
    String iccId;
    /**
     * Logical slot id is identifier of the active slot
     */
    int logicalSlotId;
    /**
     * Port active status in the slot.
     * Inactive means logical modem is no longer associated to the port.
     * Active means logical modem is associated to the port.
     */
    boolean portActive;
}
