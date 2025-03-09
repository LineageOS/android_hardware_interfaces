/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.hardware.gnss.gnss_assistance;

import android.hardware.gnss.gnss_assistance.GnssAssistance;
import android.hardware.gnss.gnss_assistance.IGnssAssistanceCallback;

/**
 * Interface used by the GNSS HAL to request the GNSS assistance data
 * (ephemeris and ionospheric corrections) from the framework.
 *
 * The GNSS chipset uses the injected assistance data in the process of computing
 * the user position for satellite position computation and error corrections.
 *
 * The framework ensures the assistance data is valid. GNSS HAL should request the
 * data when it's engine lacks valid assistance data.
 *
 * @hide
 */
@VintfStability
interface IGnssAssistanceInterface {
    /**
     * Inject the GNSS assistance into the GNSS receiver.
     *
     * @param gnssAssistance GNSS assistance.
     */
    void injectGnssAssistance(in GnssAssistance gnssAssistance);

    /**
     * Provides the callback routines to request the GNSS assistance.
     *
     * @param callback Handle to the IGnssAssistanceCallback interface.
     */
    void setCallback(in IGnssAssistanceCallback callback);
}
