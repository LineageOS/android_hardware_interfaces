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

import android.hardware.radio.config.SimSlotStatus;

/**
 * Interface declaring unsolicited radio config indications.
 * @hide
 */
@VintfStability
oneway interface IRadioConfigIndication {
    /**
     * Indicates SIM slot status change.
     *
     * This indication must be sent by the modem whenever there is any slot status change, even the
     * slot is inactive. For example, this indication must be triggered if a SIM card is inserted
     * into an inactive slot.
     *
     * @param type Type of radio indication
     * @param slotStatus new slot status info with size equals to the number of physical slots on
     *        the device
     */
    void simSlotsStatusChanged(
            in android.hardware.radio.RadioIndicationType type, in SimSlotStatus[] slotStatus);
}
