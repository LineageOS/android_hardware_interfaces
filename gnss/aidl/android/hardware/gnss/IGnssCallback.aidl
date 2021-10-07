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

import android.hardware.gnss.IGnssConfiguration;
import android.hardware.gnss.IGnssPsds;

/**
 * This interface is required for the HAL to communicate certain information
 * like status and location info back to the framework, the framework implements
 * the interfaces and passes a handle to the HAL.
 */
@VintfStability
interface IGnssCallback {
    /** Capability bit mask indicating that GNSS supports blocklisting satellites */
    const int CAPABILITY_SATELLITE_BLOCKLIST = 1 << 9;

    /** Capability bit mask indicating that GNSS supports correlation vector */
    const int CAPABILITY_CORRELATION_VECTOR = 1 << 12;

    /** Capability bit mask indicating that GNSS supports satellite PVT */
    const int CAPABILITY_SATELLITE_PVT = 1 << 13;

    /** Capability bit mask indicating that GNSS supports measurement corrections for driving */
    const int CAPABILITY_MEASUREMENT_CORRECTIONS_FOR_DRIVING = 1 << 14;

    /**
     * Callback to inform framework of the GNSS HAL implementation's capabilities.
     *
     * @param capabilities Capability parameter is a bit field of the Capability bit masks.
     */
    void gnssSetCapabilitiesCb(in int capabilities);
}
