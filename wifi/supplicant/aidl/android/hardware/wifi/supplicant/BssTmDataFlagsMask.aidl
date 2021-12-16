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

package android.hardware.wifi.supplicant;

/**
 * Bitmask of various information retrieved from BSS transition management request frame.
 */
@VintfStability
@Backing(type="int")
enum BssTmDataFlagsMask {
    /**
     * Preferred candidate list included.
     */
    WNM_MODE_PREFERRED_CANDIDATE_LIST_INCLUDED = 1 << 0,
    /**
     * Abridged.
     */
    WNM_MODE_ABRIDGED = 1 << 1,
    /**
     * Disassociation Imminent.
     */
    WNM_MODE_DISASSOCIATION_IMMINENT = 1 << 2,
    /**
     * BSS termination included.
     */
    WNM_MODE_BSS_TERMINATION_INCLUDED = 1 << 3,
    /**
     * ESS Disassociation Imminent.
     */
    WNM_MODE_ESS_DISASSOCIATION_IMMINENT = 1 << 4,
    /**
     * MBO transition reason code included.
     */
    MBO_TRANSITION_REASON_CODE_INCLUDED = 1 << 5,
    /**
     * MBO retry delay time included.
     */
    MBO_ASSOC_RETRY_DELAY_INCLUDED = 1 << 6,
    /**
     * MBO cellular data connection preference value included.
     */
    MBO_CELLULAR_DATA_CONNECTION_PREFERENCE_INCLUDED = 1 << 7,
}
