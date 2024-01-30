/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"),
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

package android.hardware.radio.ims;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
@Backing(type="int")
enum SuggestedAction {
    /** Default value */
    NONE,
    /**
     * Indicates that the IMS registration is failed with fatal error such as 403 or 404
     * on all P-CSCF addresses. The radio shall block the current PLMN or disable
     * the RAT as per the carrier requirements.
     */
    TRIGGER_PLMN_BLOCK,
    /**
     * Indicates that the IMS registration on current PLMN failed multiple times.
     * The radio shall block the current PLMN or disable the RAT during EPS or 5GS mobility
     * management timer value as per the carrier requirements.
     */
    TRIGGER_PLMN_BLOCK_WITH_TIMEOUT,
    /**
     * Indicates that the IMS registration on current RAT failed multiple times.
     * The radio shall block the current RAT and search for other available RATs in the
     * background. If no other RAT is available that meets the carrier requirements, the
     * radio may remain on the current RAT for internet service. The radio clears all
     * RATs marked as unavailable if {@link IRadioIms#updateImsRegistrationInfo()} API
     * with REGISTERED state is invoked.
     */
    TRIGGER_RAT_BLOCK,
    /**
     * Indicates that the radio clears all RATs marked as unavailable and tries to find
     * an available RAT that meets the carrier requirements.
     */
    TRIGGER_CLEAR_RAT_BLOCK,
}
