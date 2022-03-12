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

package android.hardware.gnss.measurement_corrections;

import android.hardware.gnss.measurement_corrections.IMeasurementCorrectionsCallback;
import android.hardware.gnss.measurement_corrections.MeasurementCorrections;

/**
 * Interface for measurement corrections support.
 *
 * @hide
 */
@VintfStability
interface IMeasurementCorrectionsInterface {
    /**
     * Injects measurement corrections to be used by the HAL to improve the GNSS location output.
     *
     * These are NOT to be used to adjust the IGnssMeasurementCallback output values -
     * those remain raw, uncorrected measurements.
     *
     * In general, these are injected when conditions defined by the platform are met, such as when
     * GNSS Location is being requested at a sufficiently high accuracy, based on the capabilities
     * of the GNSS chipset as reported in the IGnssCallback.
     *
     * @param corrections The computed corrections to be used by the HAL.
     */
    void setCorrections(in MeasurementCorrections corrections);

    /**
     * Opens the interface and provides the callback routines to the implementation of this
     * interface.
     *
     * @param callback Callback interface for IMeasurementCorrections.
     */
    void setCallback(in IMeasurementCorrectionsCallback callback);
}
