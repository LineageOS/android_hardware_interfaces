/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.hardware.gnss;

import android.hardware.gnss.GnssPowerStats;

/**
 * The callback interface to report GNSS Power Indication from the HAL.
 */
@VintfStability
interface IGnssPowerIndicationCallback {

    /** Capability bit mask indicating GNSS supports totalEnergyMilliJoule. */
    const int CAPABILITY_TOTAL = 1 << 0;

    /** Capability bit mask indicating GNSS supports singlebandTrackingModeEnergyMilliJoule. */
    const int CAPABILITY_SINGLEBAND_TRACKING = 1 << 1;

    /** Capability bit mask indicating GNSS supports multibandTrackingModeEnergyMilliJoule. */
    const int CAPABILITY_MULTIBAND_TRACKING = 1 << 2;

    /** Capability bit mask indicating GNSS supports singlebandAcquisitionModeEnergyMilliJoule. */
    const int CAPABILITY_SINGLEBAND_ACQUISITION = 1 << 3;

    /** Capability bit mask indicating GNSS supports multibandAcquisitionModeEnergyMilliJoule. */
    const int CAPABILITY_MULTIBAND_ACQUISITION = 1 << 4;

    /** Capability bit mask indicating GNSS supports otherModesEnergyMilliJoule. */
    const int CAPABILITY_OTHER_MODES = 1 << 5;

    /**
     * Callback to inform framework the Power Indication specific capabilities of the GNSS HAL
     * implementation.
     *
     * The GNSS HAL must call this method immediately after the framework opens the
     * IGnssPowerIndication interface.
     *
     * @param capabilities Bitmask of CAPABILITY_* specifying the supported GNSS Power Indication
     * capabilities.
     */
    void setCapabilitiesCb(in int capabilities);

    /**
     * Callback for the HAL to pass a GnssPowerStats structure back to the client.
     *
     * @param gnssPowerStats GNSS power statistics since boot.
     */
    oneway void gnssPowerStatsCb(in GnssPowerStats gnssPowerStats);
}