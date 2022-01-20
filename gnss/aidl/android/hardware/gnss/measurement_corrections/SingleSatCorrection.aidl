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

import android.hardware.gnss.GnssConstellationType;
import android.hardware.gnss.measurement_corrections.ReflectingPlane;

/**
 * A struct with measurement corrections for a single visible satellites
 *
 * The bit mask singleSatCorrectionFlags indicates which correction values are valid in the struct
 */
@VintfStability
parcelable SingleSatCorrection {
    /** Bit mask to indicate which values are valid in a SingleSatCorrection object. */
    /** GnssSingleSatCorrectionFlags has valid satellite-is-line-of-sight-probability field. */
    const int SINGLE_SAT_CORRECTION_HAS_SAT_IS_LOS_PROBABILITY = 0x0001;
    /** GnssSingleSatCorrectionFlags has valid Excess Path Length field. */
    const int SINGLE_SAT_CORRECTION_HAS_EXCESS_PATH_LENGTH = 0x0002;
    /** GnssSingleSatCorrectionFlags has valid Excess Path Length Uncertainty field. */
    const int SINGLE_SAT_CORRECTION_HAS_EXCESS_PATH_LENGTH_UNC = 0x0004;
    /** GnssSingleSatCorrectionFlags has valid Reflecting Plane field. */
    const int SINGLE_SAT_CORRECTION_HAS_REFLECTING_PLANE = 0x0008;

    /** Contains GnssSingleSatCorrectionFlags bits. */
    int singleSatCorrectionFlags;

    /**
     * Defines the constellation of the given satellite.
     */
    GnssConstellationType constellation;

    /**
     * Satellite vehicle ID number, as defined in GnssSvInfo::svid
     */
    int svid;

    /**
     * Carrier frequency of the signal to be corrected, for example it can be the
     * GPS center frequency for L1 = 1,575,420,000 Hz, varying GLO channels, etc.
     *
     * For a receiver with capabilities to track multiple frequencies for the same satellite,
     * multiple corrections for the same satellite may be provided.
     */
    long carrierFrequencyHz;

    /**
     * The probability that the satellite is estimated to be in Line-of-Sight condition at the given
     * location.
     */
    float probSatIsLos;

    /**
     * Excess path length to be subtracted from pseudorange before using it in calculating location.
     *
     * Note this value is NOT to be used to adjust the GnsseasurementCallback outputs.
     */
    float excessPathLengthMeters;

    /** Error estimate (1-sigma) for the Excess path length estimate */
    float excessPathLengthUncertaintyMeters;

    /**
     * Defines the reflecting plane characteristics such as location and azimuth
     *
     * The value is only valid if HAS_REFLECTING_PLANE flag is set. An invalid reflecting plane
     * means either reflection planes serving is not supported or the satellite signal has gone
     * through multiple reflections.
     */
    ReflectingPlane reflectingPlane;
}
