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
}
