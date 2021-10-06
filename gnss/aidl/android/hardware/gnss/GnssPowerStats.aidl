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

import android.hardware.gnss.ElapsedRealtime;

/**
 * Cumulative GNSS power statistics since boot.
 */
@VintfStability
parcelable GnssPowerStats {
    /**
     * Timing information of the GnssPowerStats synchronized with SystemClock.elapsedRealtimeNanos()
     * clock.
     */
    ElapsedRealtime elapsedRealtime;

    /**
     * Total GNSS energy consumption in milli-joules (mWatt-seconds).
     */
    double totalEnergyMilliJoule;

    /**
     * Total energy consumption in milli-joules (mWatt-seconds) for which the GNSS engine is
     * tracking signals of a single frequency band.
     */
    double singlebandTrackingModeEnergyMilliJoule;

    /**
     * Total energy consumption in milli-joules (mWatt-seconds) for which the GNSS engine is
     * tracking signals of multiple frequency bands.
     */
    double multibandTrackingModeEnergyMilliJoule;

    /**
     * Total energy consumption in milli-joules (mWatt-seconds) for which the GNSS engine is
     * acquiring signals of a single frequency band.
     */
    double singlebandAcquisitionModeEnergyMilliJoule;

    /**
     * Total energy consumption in milli-joules (mWatt-seconds) for which the GNSS engine is
     * acquiring signals of multiple frequency bands.
     */
    double multibandAcquisitionModeEnergyMilliJoule;

    /**
     * Total energy consumption in milli-joules (mWatt-seconds) for which the GNSS engine is
     * operating in each of the vendor-specific power modes.
     */
    double[] otherModesEnergyMilliJoule;
}