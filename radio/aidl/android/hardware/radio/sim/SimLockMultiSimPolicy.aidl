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
@Backing(type="int")
@JavaDerive(toString=true)
enum SimLockMultiSimPolicy {

    /**
     * Indicates that configuration applies to each slot independently.
     */
    NO_MULTISIM_POLICY,

    /**
     * Indicates that any SIM card can be used as far as one valid card is present in the device.
     * For the modem, a SIM card is valid when its content (i.e. MCC, MNC, GID, SPN) matches the
     * carrier restriction configuration.
     */
    ONE_VALID_SIM_MUST_BE_PRESENT,

    /**
     * Indicates that the SIM lock policy applies uniformly to all sim slots.
     */
    APPLY_TO_ALL_SLOTS,

    /**
     * The SIM lock configuration applies exclusively to sim slot 1, leaving
     * all other sim slots unlocked irrespective of the SIM card in slot 1
     */
    APPLY_TO_ONLY_SLOT_1,

    /**
     * Valid sim cards must be present on sim slot1 in order
     * to use other sim slots.
     */
    VALID_SIM_MUST_PRESENT_ON_SLOT_1,

    /**
     * Valid sim card must be present on slot1 and it must be in full service
     * in order to use other sim slots.
     */
    ACTIVE_SERVICE_ON_SLOT_1_TO_UNBLOCK_OTHER_SLOTS,

    /**
     * Valid sim card be present on any slot and it must be in full service
     * in order to use other sim slots.
     */
    ACTIVE_SERVICE_ON_ANY_SLOT_TO_UNBLOCK_OTHER_SLOTS,

    /**
     * Valid sim cards must be present on all slots. If any SIM cards become
     * invalid then device would set other SIM cards as invalid as well.
     */
    ALL_SIMS_MUST_BE_VALID,

    /**
     * In case there is no match policy listed above.
     */
    SLOT_POLICY_OTHER
}
