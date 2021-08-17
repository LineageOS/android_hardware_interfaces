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

import android.hardware.gnss.IGnssMeasurementCallback;

/**
 * Extended interface for GNSS Measurement support.
 */
@VintfStability
interface IGnssMeasurementInterface {
    /**
     * Initializes the interface and registers the callback routines with the HAL. After a
     * successful call to 'setCallback' the HAL must begin to provide updates at an average
     * output rate of 1Hz (occasional intra-measurement time offsets in the range from 0-2000msec
     * can be tolerated.)
     *
     * @param callback Handle to GnssMeasurement callback interface.
     * @param enableFullTracking If true, GNSS chipset must switch off duty cycling. In such mode
     *     no clock discontinuities are expected and, when supported, carrier phase should be
     *     continuous in good signal conditions. All non-blocklisted, healthy constellations,
     *     satellites and frequency bands that the chipset supports must be reported in this mode.
     *     The GNSS chipset is allowed to consume more power in this mode. If false, API must
     *     optimize power via duty cycling, constellations and frequency limits, etc.
     *
     * @param enableCorrVecOutputs If true, enable correlation vectors as part of the raw GNSS
     *     measurements outputs. If false, disable correlation vectors.
     *
     * Returns ok() if successful. Returns ERROR_ALREADY_INIT if a callback has already been
     * registered without a corresponding call to 'close'. Returns ERROR_GENERIC for any other
     * error. The HAL must not generate any other updates upon returning this error code.
     */
    void setCallback(in IGnssMeasurementCallback callback, in boolean enableFullTracking,
                     in boolean enableCorrVecOutputs);

    /**
     * Stops updates from the HAL, and unregisters the callback routines. After a call to close(),
     * the previously registered callbacks must be considered invalid by the HAL.
     *
     * If close() is invoked without a previous setCallback, this function must perform
     * no work.
     */
    void close();
}