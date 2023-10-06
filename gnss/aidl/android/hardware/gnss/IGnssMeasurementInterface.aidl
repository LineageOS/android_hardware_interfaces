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
 *
 * @hide
 */
@VintfStability
interface IGnssMeasurementInterface {
    /**
     * Options specifying the GnssMeasurement request.
     */
    @VintfStability
    parcelable Options {
        /**
         * Enable full tracking mode.
         *
         * If true, GNSS chipset must switch off duty cycling. In such mode no clock discontinuities
         * are expected and, when supported, carrier phase should be continuous in good signal
         * conditions. All non-blocklisted, healthy constellations, satellites and frequency bands
         * that are meaningful to positioning accuracy must be tracked and reported in this mode.
         * The GNSS chipset is allowed to consume more power in this mode. If false, API must
         * optimize power via duty cycling, constellations and frequency limits, etc.
         */
        boolean enableFullTracking;

        /**
         * Enable Correlation Vector outputs.
         *
         * If true, enable correlation vectors as part of the raw GNSS measurements outputs. If
         * false, disable correlation vectors.
         */
        boolean enableCorrVecOutputs;

        /**
         * Time interval between the reported measurements in milliseconds.
         *
         * When there is no concurrent location and measurement requests, the GNSS chipset must
         * report measurements at as close as possible to the requested rate, as is supported by the
         * implementation.
         *
         * When there are concurrent location and measurement requests, the GNSS chipset must report
         * measurements at the same or a faster rate than the requested. In the concurrency cases,
         * all the available measurements must be reported to the framework.
         *
         * For cases where concurrently serving the location and the measurement requests would not
         * consume more power than only the measurement request, the faster rate of the 2 requests
         * must be chosen. Otherwise, it is recommended that the GNSS chipset minimizes the power
         * consumption with appropriate location and measurement intervals to satisfy both requests.
         * For example, for 2-sec measurement interval request and 7-sec location interval request,
         * the GNSS chipset is recommended to run the measurement engine with 2-sec interval and the
         * location engine with 6-sec interval.
         */
        int intervalMs;
    }

    /**
     * Initializes the interface and registers the callback routines with the HAL. After a
     * successful call to 'setCallback' the HAL must begin to provide updates at an average
     * output rate of 1Hz (occasional intra-measurement time offsets in the range from 0-2000msec
     * can be tolerated.)
     *
     * If setCallback() is invoked without a previous close(), the HAL must use the new callback
     * and parameters to provide updates.
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

    /**
     * Initializes the interface and registers the callback routines with the HAL.
     *
     * If setCallbackWithOptions() is invoked without a previous close(), the HAL must use the new
     * callback and options to provide updates.
     *
     * @param options See Options definition.
     */
    void setCallbackWithOptions(in IGnssMeasurementCallback callback, in Options options);
}
