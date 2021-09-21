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

@VintfStability
parcelable SimSlotStatus {
    boolean cardActive;
    /**
     * Card state in the physical slot. Values are CardStatus.[STATE_ABSENT, STATE_PRESENT,
     * STATE_ERROR, STATE_RESTRICTED].
     */
    int cardState;
    /**
     * Slot state Active/Inactive
     */
    int slotState;
    /**
     * An Answer To Reset (ATR) is a message output by a Smart Card conforming to ISO/IEC 7816
     * standards, following electrical reset of the card's chip. The ATR conveys information about
     * the communication parameters proposed by the card, and the card's nature and state.
     *
     * This data is applicable only when cardState is CardStatus.STATE_PRESENT.
     */
    String atr;
    int logicalSlotId;
    /**
     * Integrated Circuit Card IDentifier (ICCID) is Unique Identifier of the SIM CARD. File is
     * located in the SIM card at EFiccid (0x2FE2) as per ETSI 102.221. The ICCID is defined by
     * the ITU-T recommendation E.118 ISO/IEC 7816.
     *
     * This data is applicable only when cardState is CardStatus.STATE_PRESENT.
     */
    String iccid;
    /**
     * The EID is the eUICC identifier. The EID shall be stored within the ECASD and can be
     * retrieved by the Device at any time using the standard GlobalPlatform GET DATA command.
     *
     * This data is mandatory and applicable only when cardState is CardStatus.STATE_PRESENT and SIM
     * card supports eUICC.
     */
    String eid;
}
